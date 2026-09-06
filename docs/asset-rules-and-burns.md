# Asset rules and where assets get burned

Reference for anyone touching the asset transfer path.  Written after issue 29, where
`sendasset` destroyed a holder's entire balance because the wallet built a transaction that
violated a royalty rule.

---

## Why violations burn rather than fail

DigiAssets are an overlay on DigiByte.  By the time a node sees a transaction it is **already
confirmed on the chain** and the DigiByte UTXOs carrying the assets are spent.

That leaves no way to "reject" it.  The assets cannot stay on the inputs, because those no
longer exist, and they cannot go to the outputs, because that is the thing the rules just
forbade.  The only remaining state is not existing.

So burning is not a punishment chosen over some gentler option — it is the only coherent
outcome available to an indexer that can observe the chain but not change it.  A node can only
help by refusing to *create* an invalid transaction in the first place, which is why prevention
lives in the wallet.

**Everything that follows depends on every node applying these rules identically.**  Nodes agree
on who owns what because they all derive it from the same chain with the same logic, not because
they communicate.  Changing any rule below changes the answer for transactions already on the
chain, so it can only be done behind an activation height.

---

## The rules

All are checked by `DigiAsset::checkRulesPass()` (`src/DigiAsset.cpp`), which runs on decode for
anything that is not an issuance.  Any failure throws `exceptionRuleFailed` and burns — see the
next section.

| Rule | Requires | Can a wallet-built transfer satisfy it? |
|---|---|---|
| **Signers** | signatures totalling the required weight | **No** — the wallet cannot collect them |
| **Royalty** | an output paying each royalty address | **No** — nothing in the transfer path adds it |
| **KYC / geofence** | every receiving address is KYC'd and in an allowed country | **Yes** — send to an allowed address |
| **Expiry** | the asset has not expired | **No, once expired** — no transfer passes again |
| **Vote** | every receiving address is a valid vote address | **Yes** — send to a vote address |
| **Deflation** (required burn) | at least the required amount burned | **No** — the wallet adds no burn |

`addRuleOutputs()` is called from `issueasset` **only**.  No part of the transfer path adds rule
outputs, which is the root of issue 29.

### Royalty counting

Worth understanding before changing it:

```cpp
size_t count = -1;                       // so the loop yields "outputs holding the asset, minus one"
for (outputs) if (holds this asset) count++;
if (count < 1) count = 1;                // but never charge for fewer than one recipient
```

- **Change is not charged.** Starting at `-1` drops one output from the count, which is the
  sender's change.
- **The minimum is one.** Otherwise a sender could route everything to themselves as "change",
  move nothing on paper, and pay no royalty.
- **A complete burn leaves `count` wrapped at `SIZE_MAX`.** With no outputs holding the asset the
  loop never runs, and `SIZE_MAX < 1` is false so the floor does not catch it.  The multiplication
  overflows into an unpayable figure, the rule fails, and everything burns — which is exactly what
  a complete burn was asking for.  The error state is the correct state, so no extra arithmetic is
  spent avoiding it.
- **A partial burn is charged like a send.** One output holds the asset (the change), so `count`
  is 1 and a royalty is genuinely owed.  `burnasset` no more adds that output than `sendasset`
  does, so an unguarded partial burn destroys the remainder.

---

## Where assets get burned

`src/DigiByteTransaction.cpp` unless noted.  `isUnintentionalBurn()` reports all but the first.

| # | Situation | Where | What is lost |
|---|---|---|---|
| 1 | **Intentional burn** — transfer instruction targeting output 31 (`opcode >= 0x20`) | `addAssetBurn()`, decode | only what was marked |
| 2 | **Asset UTXO spent as plain DigiByte** — no asset instruction at all | `isUnintentionalBurn()` — `_txType == STANDARD && _assetFound` | everything on the inputs |
| 3 | **Asset-bearing inputs in an issuance** — assets are not transferred during issuance | decode, `if (_assetFound) _unintentionalBurn = true` | everything on the inputs |
| 4 | **Rule violation** | the `exceptionRuleFailed` handler | **every asset output, change included** |
| 5 | **Malformed transfer instruction** — any exception while decoding | the decode `catch`, clears all outputs | everything |
| 6 | **Assets assigned to an OP_RETURN output** (`address == ""`) | post-decode sweep | that output's assets |
| 7 | **Inputs never assigned to an output** | transfer encoding | the unassigned remainder |

Case 4 is the one that surprises people: the change output dies with the rest.  That is
deliberate.  If change survived a violation, a sender could declare everything as change and
sidestep the royalty entirely — the rule would be optional for anyone willing to shape their
transaction that way.

The transaction builder already refuses case 3 up front rather than letting you build it:
*"Asset bearing inputs can not be used in an issuance(they would be burned)"*.

---

## Prevention, which is the wallet's job

`AssetWallet::assertTransferableAsset()` (`src/AssetWallet.h`) is called from
`selectAssetInputs()` before any input is chosen, so it covers `sendasset`, `sendmanyassets` and
`burnasset` — its only callers.

It refuses only the rules **no** transfer could ever satisfy: royalty, required burn, required
signer, and already-expired.  KYC and vote restrictions are deliberately allowed through, because
a send to an allowed address is legal and has to keep working.

`reissueasset` does not go through `selectAssetInputs`.  It walks `getWalletUTXOs(1)` directly and
selects only inputs where `utxo.assets.empty()`, so it spends no asset inputs and **cannot**
destroy a holding this way.

A rule violation now also writes a `WARNING` naming the transaction, block and rule.  Before that,
a burn of this kind happened in complete silence, which is why issue 29 was found by losing assets
on mainnet rather than by reading a log.
