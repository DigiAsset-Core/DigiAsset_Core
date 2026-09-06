//
// Tests for Config class — no external dependencies required.
//

#include "Config.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static string writeTempConfig(const string& content) {
    char path[] = "/tmp/digiasset_config_test_XXXXXX";
    int fd = mkstemp(path);
    if (fd == -1) throw runtime_error("mkstemp failed");
    FILE* f = fdopen(fd, "w");
    fputs(content.c_str(), f);
    fclose(f);
    return string(path);
}

// ─────────────────────────────────────────────────────────────────────────────
// Empty constructor + setters / getters
// ─────────────────────────────────────────────────────────────────────────────

TEST(Config, emptyConstructorAndSetters) {
    Config cfg;

    cfg.setString("host", "localhost");
    EXPECT_EQ(cfg.getString("host"), "localhost");

    cfg.setInteger("port", 8332);
    EXPECT_EQ(cfg.getInteger("port"), 8332);

    cfg.setBool("debug", true);
    EXPECT_TRUE(cfg.getBool("debug"));

    cfg.setBool("verbose", false);
    EXPECT_FALSE(cfg.getBool("verbose"));
}

TEST(Config, getPlaceholderKeys) {
    // example.cfg writes the per pool options with a # standing in for the pool number.  Copied
    // over as is they parse as keys of their own that nothing ever reads, so the daemon has to be
    // able to spot them
    const string path = writeTempConfig(
            "psp#subscribe=0\n"
            "psp#payout=_psppayout\n"
            "psp1visible=1\n"
            "rpcallow*=1\n"
            "rpcpassword=has#in#value\n");
    Config cfg(path);
    vector<string> found = cfg.getPlaceholderKeys();
    remove(path.c_str());

    ASSERT_EQ(found.size(), 2u);
    EXPECT_NE(std::find(found.begin(), found.end(), "psp#subscribe"), found.end());
    EXPECT_NE(std::find(found.begin(), found.end(), "psp#payout"), found.end());
}

TEST(Config, getPlaceholderKeys_noneOnACleanConfig) {
    const string path = writeTempConfig(
            "#psp#subscribe=0 is only a note\n"
            "psp0subscribe=0\n"
            "psp1subscribe=1\n");
    Config cfg(path);
    vector<string> found = cfg.getPlaceholderKeys();
    remove(path.c_str());

    EXPECT_TRUE(found.empty());
}

TEST(Config, isKey) {
    Config cfg;
    EXPECT_FALSE(cfg.isKey("missing"));

    cfg.setString("name", "test");
    EXPECT_TRUE(cfg.isKey("name"));
    EXPECT_TRUE(cfg.isKey("name", Config::STRING));

    cfg.setInteger("count", 5);
    EXPECT_TRUE(cfg.isKey("count", Config::INTEGER));
    EXPECT_TRUE(cfg.isKey("count", Config::STRING)); // STRING always true
}

// ─────────────────────────────────────────────────────────────────────────────
// Missing key exceptions
// ─────────────────────────────────────────────────────────────────────────────

TEST(Config, getString_missingKey_throws) {
    Config cfg;
    EXPECT_THROW(cfg.getString("missing"), Config::exceptionCorruptConfigFile_Missing);
}

TEST(Config, getInteger_missingKey_throws) {
    Config cfg;
    EXPECT_THROW(cfg.getInteger("missing"), Config::exceptionCorruptConfigFile_Missing);
}

TEST(Config, getBool_missingKey_throws) {
    Config cfg;
    EXPECT_THROW(cfg.getBool("missing"), Config::exceptionCorruptConfigFile_Missing);
}

// ─────────────────────────────────────────────────────────────────────────────
// Default value overloads
// ─────────────────────────────────────────────────────────────────────────────

TEST(Config, defaultValues) {
    Config cfg;
    EXPECT_EQ(cfg.getString("key", "fallback"), "fallback");
    EXPECT_EQ(cfg.getInteger("key", 99), 99);
    EXPECT_EQ(cfg.getBool("key", true), true);
    EXPECT_EQ(cfg.getBool("key", false), false);

    // Existing key should override default
    cfg.setString("key", "real");
    EXPECT_EQ(cfg.getString("key", "fallback"), "real");
}

// ─────────────────────────────────────────────────────────────────────────────
// Type mismatch
// ─────────────────────────────────────────────────────────────────────────────

TEST(Config, getInteger_wrongType_throws) {
    Config cfg;
    cfg.setString("host", "localhost");
    EXPECT_THROW(cfg.getInteger("host"), Config::exceptionCorruptConfigFile_WrongType);
}

TEST(Config, getBool_wrongType_throws) {
    Config cfg;
    cfg.setString("host", "localhost");
    EXPECT_THROW(cfg.getBool("host"), Config::exceptionCorruptConfigFile_WrongType);
}

// ─────────────────────────────────────────────────────────────────────────────
// Bool formats — "true"/"false"/"0"/"1"
// ─────────────────────────────────────────────────────────────────────────────

TEST(Config, boolFormats) {
    Config cfg;

    cfg.setString("a", "true");
    EXPECT_TRUE(cfg.getBool("a"));

    cfg.setString("b", "false");
    EXPECT_FALSE(cfg.getBool("b"));

    cfg.setString("c", "1");
    EXPECT_TRUE(cfg.getBool("c"));

    cfg.setString("d", "0");
    EXPECT_FALSE(cfg.getBool("d"));

    // "2" is not a valid bool value
    cfg.setString("e", "2");
    EXPECT_THROW(cfg.getBool("e"), Config::exceptionCorruptConfigFile_WrongType);
}

// ─────────────────────────────────────────────────────────────────────────────
// Map getters
// ─────────────────────────────────────────────────────────────────────────────

TEST(Config, getStringMap) {
    Config cfg;
    cfg.setString("peer.host1", "10.0.0.1");
    cfg.setString("peer.host2", "10.0.0.2");
    cfg.setString("other.key", "ignored");

    auto m = cfg.getStringMap("peer.");
    EXPECT_EQ(m.size(), 2u);
    EXPECT_EQ(m.at("host1"), "10.0.0.1");
    EXPECT_EQ(m.at("host2"), "10.0.0.2");
    EXPECT_EQ(m.count("key"), 0u);
}

TEST(Config, getIntegerMap) {
    Config cfg;
    cfg.setInteger("score.alice", 10);
    cfg.setInteger("score.bob", 20);
    cfg.setString("score.bad", "notanint");

    auto m = cfg.getIntegerMap("score.");
    EXPECT_EQ(m.size(), 2u);
    EXPECT_EQ(m.at("alice"), 10);
    EXPECT_EQ(m.at("bob"), 20);
    EXPECT_EQ(m.count("bad"), 0u); // non-integer skipped silently
}

TEST(Config, getBoolMap) {
    Config cfg;
    cfg.setBool("flag.a", true);
    cfg.setBool("flag.b", false);
    cfg.setString("flag.c", "notabool");

    auto m = cfg.getBoolMap("flag.");
    EXPECT_EQ(m.size(), 2u);
    EXPECT_TRUE(m.at("a"));
    EXPECT_FALSE(m.at("b"));
    EXPECT_EQ(m.count("c"), 0u); // non-bool skipped silently
}

TEST(Config, setStringMap_and_getStringMap) {
    Config cfg;
    map<string, string> data = {{"x", "1"}, {"y", "2"}};
    cfg.setStringMap("ns.", data);

    auto result = cfg.getStringMap("ns.");
    EXPECT_EQ(result, data);
}

TEST(Config, setIntegerMap_and_getIntegerMap) {
    Config cfg;
    map<string, int> data = {{"a", 100}, {"b", 200}};
    cfg.setIntegerMap("num.", data);

    auto result = cfg.getIntegerMap("num.");
    EXPECT_EQ(result, data);
}

// ─────────────────────────────────────────────────────────────────────────────
// write() + file constructor round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST(Config, writeAndReload) {
    Config cfg;
    cfg.setString("rpcuser", "admin");
    cfg.setInteger("rpcport", 14022);
    cfg.setBool("enabled", true);

    string path = writeTempConfig(""); // just get a valid tmp path
    cfg.write(path);

    Config loaded(path);
    EXPECT_EQ(loaded.getString("rpcuser"), "admin");
    EXPECT_EQ(loaded.getInteger("rpcport"), 14022);
    EXPECT_TRUE(loaded.getBool("enabled"));

    remove(path.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// File parsing: comments and blank lines are ignored
// ─────────────────────────────────────────────────────────────────────────────

TEST(Config, fileParsingIgnoresCommentsAndBlanks) {
    string content = "# This is a comment\n\nkey=value\n# another comment\nnum=42\n";
    string path = writeTempConfig(content);

    Config cfg(path);
    EXPECT_EQ(cfg.getString("key"), "value");
    EXPECT_EQ(cfg.getInteger("num"), 42);
    EXPECT_FALSE(cfg.isKey("#"));

    remove(path.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// Missing file throws
// ─────────────────────────────────────────────────────────────────────────────

TEST(Config, missingFile_throws) {
    EXPECT_THROW(Config cfg("/nonexistent/path/config.cfg"), Config::exceptionConfigFileMissing);
}

// ─────────────────────────────────────────────────────────────────────────────
// clear()
// ─────────────────────────────────────────────────────────────────────────────

TEST(Config, clear) {
    Config cfg;
    cfg.setString("key", "value");
    EXPECT_TRUE(cfg.isKey("key"));
    cfg.clear();
    EXPECT_FALSE(cfg.isKey("key"));
    EXPECT_EQ(cfg.getString("key", "default"), "default");
}
