#!/usr/bin/env python3
"""
Verifies that the rewritten _stmtExchangeRatesAtHeight query in src/Database.cpp (a
MAX(height)-per-group join) returns exactly the same rows as the original query it replaced
(a ROW_NUMBER() OVER (PARTITION BY ...) window function), across many randomized datasets with
sparse/gappy height data and a wide range of query heights.

Both queries answer: "for every (address,index) pair, what is the most recent exchange rate
recorded at or before height X?" - an "as of" lookup. Holes in the data(missing heights,
groups that start late or stop early, groups with a single row, groups with no rows at all)
must not change the answer, since the query only cares about the MAX matching height per
group, not which heights exist in between.

No external dependencies - uses Python's built in sqlite3 module against an in-memory database
with the same schema/indexes as production.

Usage: python3 compare_exchange_rate_query.py
Exit code 0 = all cases matched, 1 = at least one mismatch (printed).
"""
import random
import sqlite3
import sys
import time

SCHEMA = """
CREATE TABLE "exchange" ("address" TEXT NOT NULL, "index" INTEGER NOT NULL, "height" INTEGER NOT NULL, "value" REAL NOT NULL, PRIMARY KEY("address","index","height"));
CREATE INDEX idx_exchange_address_index_height ON exchange("address", "index", "height" DESC);
CREATE INDEX idx_exchange_height ON exchange("height");
"""

# the query being replaced (Database.cpp before the fix)
OLD_QUERY = """
WITH cte AS (
  SELECT *, ROW_NUMBER() OVER (PARTITION BY [address], [index] ORDER BY height DESC) AS row_number
  FROM exchange
  WHERE height <= ?
)
SELECT [height], [address], [index], [value]
FROM cte
WHERE row_number = 1;
"""

# the replacement query (Database.cpp after the fix)
NEW_QUERY = """
SELECT e.[height], e.[address], e.[index], e.[value]
FROM exchange e
JOIN (
  SELECT [address], [index], MAX([height]) AS [height]
  FROM exchange
  WHERE height <= ?
  GROUP BY [address], [index]
) m ON e.[address] = m.[address] AND e.[index] = m.[index] AND e.[height] = m.[height];
"""


def build_db(rows):
    conn = sqlite3.connect(":memory:")
    conn.executescript(SCHEMA)
    conn.executemany("INSERT INTO exchange VALUES (?,?,?,?)", rows)
    conn.commit()
    return conn


def run(conn, query, height):
    return sorted(conn.execute(query, (height,)).fetchall())


def gen_dataset(rng, num_addresses, num_indexes, max_height, hole_probability):
    """Generates rows with deliberate holes: each (address,index) group starts late and/or
    stops early some of the time(no data at the edges of the range), and within its own
    active range, heights are visited with irregular, often large, gaps(missing entries),
    with some candidate heights additionally skipped at random(hole_probability)."""
    rows = []
    for a in range(num_addresses):
        address = f"addr{a}"
        for index in range(num_indexes):
            start = rng.randint(0, max_height // 2)
            end = rng.randint(max_height // 2, max_height)
            if start >= end:
                continue  # this group gets zero rows entirely
            height = start
            while height <= end:
                if rng.random() > hole_probability:
                    value = round(rng.uniform(0.0001, 100000.0), 8)
                    rows.append((address, index, height, value))
                height += rng.randint(1, max(2, max_height // 200))  # irregular spacing
    # a group with exactly one row
    rows.append(("addr_single_row", 0, rng.randint(0, max_height), 1.23456789))
    return rows


def heights_to_test(conn, max_height, rng):
    heights_in_data = sorted({r[0] for r in conn.execute("SELECT DISTINCT height FROM exchange")})
    test_heights = {0, max_height, max_height + 1, max_height * 2 + 1}
    if heights_in_data:
        test_heights.add(max(0, heights_in_data[0] - 1))
        test_heights.add(heights_in_data[0])
        test_heights.add(heights_in_data[-1])
        test_heights.add(heights_in_data[-1] + 1)
        for a, b in zip(heights_in_data, heights_in_data[1:]):
            if b - a > 1:
                test_heights.add((a + b) // 2)  # exact middle of a hole
                test_heights.add(a + 1)         # right after a known height
                test_heights.add(b - 1)         # right before the next known height
    for _ in range(300):
        test_heights.add(rng.randint(0, max_height + 1000))
    return sorted(test_heights)


def compare_case(conn, height):
    old = run(conn, OLD_QUERY, height)
    new = run(conn, NEW_QUERY, height)
    return old, new


def main():
    mismatches = []
    total_checks = 0

    # empty table edge case
    conn = build_db([])
    for h in (0, 1, 1_000_000):
        old, new = compare_case(conn, h)
        total_checks += 1
        if old != new:
            mismatches.append(("empty-table", h, old, new))
    conn.close()

    # randomized datasets covering a wide range of sizes, sparsity and query heights
    num_datasets = 25
    for seed in range(num_datasets):
        rng = random.Random(seed)
        max_height = rng.choice([100, 2_000, 50_000, 2_000_000, 23_000_000])
        rows = gen_dataset(
            rng,
            num_addresses=rng.randint(1, 6),
            num_indexes=rng.randint(1, 10),
            max_height=max_height,
            hole_probability=rng.choice([0.0, 0.1, 0.5, 0.9, 0.99]),
        )
        conn = build_db(rows)
        for h in heights_to_test(conn, max_height, rng):
            old, new = compare_case(conn, h)
            total_checks += 1
            if old != new:
                mismatches.append((f"seed={seed} rows={len(rows)} max_height={max_height}", h, old, new))
        conn.close()

    print(f"Checked {total_checks} query heights across {num_datasets} randomized datasets "
          f"(plus the empty-table case).")
    if mismatches:
        print(f"FAIL: {len(mismatches)} mismatch(es) found:\n")
        for label, h, old, new in mismatches[:20]:
            print(f"  [{label}] height={h}")
            print(f"    old ({len(old)} rows): {old}")
            print(f"    new ({len(new)} rows): {new}\n")
        sys.exit(1)

    print("PASS: old and new queries returned identical results in every case, including "
          "sparse/gappy data, groups with a single row, groups with no rows at all in parts "
          "of the range, exact-match heights, gap midpoints, and the empty table.")

    # bonus: not a correctness check, just confirms the perf win holds up at realistic scale
    print("\n--- performance sanity check (not a correctness test) ---")
    rng = random.Random("perf")
    max_height = 23_000_000
    rows = []
    for a in range(2):
        address = f"addr{a}"
        for index in range(10):
            height = 0
            while height <= max_height:
                rows.append((address, index, height, round(rng.uniform(0.0001, 100000.0), 8)))
                height += rng.randint(50, 500)  # ~ similar density to production's 2.5M rows
    conn = build_db(rows)
    print(f"synthetic dataset rows: {len(rows)}")
    for label, query in (("old (window function)", OLD_QUERY), ("new (max-join)", NEW_QUERY)):
        t0 = time.time()
        run(conn, query, max_height)
        print(f"  {label}: {time.time() - t0:.3f}s")
    conn.close()


if __name__ == "__main__":
    main()
