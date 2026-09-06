//
// Created by mctrivia on 28/12/24.
//

#ifndef DIGIASSET_CORE_DIGIASSETCONSTANTS_H
#define DIGIASSET_CORE_DIGIASSETCONSTANTS_H


#include "DigiAssetTypes.h"
namespace DigiAssetConstants {

    /**
     * Number of sats placed on outputs that carry assets when building new asset transactions.
     * Must clear DigiByte Core's dust threshold or the wallet refuses to fund the tx
     * ("Transaction amount too small"): v8.22 uses DUST_RELAY_TX_FEE=30000 sat/kvB, giving
     * ~2940 sats for P2WPKH and ~5460 for P2PKH outputs.  10000 clears all standard types.
     * (Historical on-chain asset txs carry 600 sats; that only matters when decoding.)
     */
    const uint64_t DIGIBYTE_DUST = 10000;


        /**
     * List of addresses that can be used for voting without hard encoding the addresses you wish to use in to the asset issuance.
     * You don't need to use these but it makes your asset issuance much smaller if you do and garbage collection is automatically
     * run on these addresses to keep the chain UTXO list small.
     */
    const size_t standardExchangeRatesCount = 20;
    const ExchangeRate standardExchangeRates[] = {
            {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
             .index = 0,
             .name = "CAD"},
            {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
             .index = 1,
             .name = "USD"},
            {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
             .index = 2,
             .name = "EUR"},
            {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
             .index = 3,
             .name = "GBP"},
            {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
             .index = 4,
             .name = "AUD"},
            {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
             .index = 5,
             .name = "JPY"},
            {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
             .index = 6,
             .name = "CNY"},
            {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
             .index = 7,
             .name = "TRY"},
            {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
             .index = 8,
             .name = "BRL"},
            {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
             .index = 9,
             .name = "CHF"},
            {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
             .index = 0,
             .name = "BTC"},
            {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
             .index = 1,
             .name = "ETH"},
            {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
             .index = 2,
             .name = "LTC"},
            {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
             .index = 3,
             .name = "DCR"},
            {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
             .index = 4,
             .name = "ZIL"},
            {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
             .index = 5,
             .name = "RVN"},
            {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
             .index = 6,
             .name = "XVG"},
            {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
             .index = 7,
             .name = "RDD"},
            {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
             .index = 8,
             .name = "NXS"},
            {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
             .index = 9,
             .name = "POT"}};

    const size_t standardVoteCount = 50;
    const std::string standardVoteAddresses[] = {"D8LWk1fGksGDxZai17A5wQUVsRiV69Nk7J",
                                                 "DBJNvWeirccgeAdZn9gV5otheutdthzWxx",
                                                 "D9zaWjGHuVNB32G7Pf5BMmtvDifdoS3Wsq",
                                                 "DEKQEMFHTc1M8Gs4xvY6paZ5RKtE1cbqNp",
                                                 "D8jnQigMYwhrB6Zjs73deF5RKprUdX5uvd",
                                                 "DELKWiuSj86pMfDb7aDaAUhLYG8D7H6JVj",
                                                 "DHUg85Pbc6mDK3y7kaWmsqjRaWfRVGys2U",
                                                 "D9rxXUhaDxku4ZhdkLtyZzmmGG5ViUAYds",
                                                 "DNehqnpzLWnv7vTTxkbHsneajBEzGjvLo2",
                                                 "DAKiRnvVCfD4imp5A41tCeoZkezzkPXB4C",
                                                 "D69jwFMuawBkG1hii1muQESFrbZFenZrmL",
                                                 "D7PJqwFSLmCURDNnd5cc9Ham856GwDQ9zy",
                                                 "DFHa9HQ9BDHuDKmBPvPzBE5dsLm85prUd4",
                                                 "DAJMr7m4ZyaCRa9Y1o8pMaAPViBhSZTENs",
                                                 "DFXqwRzai3Khd3n1uRaYgZTq1BhAUhyu3m",
                                                 "DSTKiCYQqpvrXME3rEFeYEsH3dZHCPU8ez",
                                                 "DG1rJMg6zCMoiptWeEozxpuVWKGmZkiHTf",
                                                 "DRgWqHV6d7HSxYhA5bCMvtLhuS3kbRYKo3",
                                                 "DD9kssWTzT8s5fv4Xg3MthRNCT7RtawQSw",
                                                 "DLLYN7hv535nXzpvZv25ySG8GdsfYNk1Bx",
                                                 "DJQEaiT39GyJgCJK7noarscutoeWHXMLaM",
                                                 "DEkaR4NfvWx3bq1MBw2nTcTP2JEPxKyaBX",
                                                 "DN9vVGNYzbjqTGRRGXkjiGVTpuKRz1eYe3",
                                                 "D6kCF8PDhwdPzSg3xeUmrDzVK9eK3nuEJj",
                                                 "DR3F3WE78aJmHvyGA35NjkLLf5F9X8eKaz",
                                                 "DSMdwgWYbEpPNQJ2Hs9Y89JqNmQdiwhWaq",
                                                 "DGJCxLgqW2sbhomNZvsDGsjam8pnY2b7uA",
                                                 "DAaQuGSbvRQA2B7zzbrQ3SRRYC9qaQVZch",
                                                 "DJbcjvGf7wzQaAQQ9GpHP1menk6jHyCsW4",
                                                 "DC5vxafEZQeqpqDawTyDx7nBW81V6LfrE5",
                                                 "DKJhzwe5PzFQuUdrzJR9gUfkvk4jzvUrZU",
                                                 "DRX7r83LHBf1cWKnBoAd8q1iN6UP833kPx",
                                                 "DShw4ZaRmW9fyWnP3umEyZ9KyJHWR9v6BD",
                                                 "DK9zdFCv9yz3C7jVbnTbVCZgGAd8S1Xqxk",
                                                 "DG2Gv2aZRALtMkKtaJEQr75bFqsL3JbKmB",
                                                 "D9zt7Xb1RgBepPrrYSRPv6N6YCUcS5CRx6",
                                                 "DAgRoBgYaDx7g6JPRZyufvFQAQc4zdjaP1",
                                                 "DPLi14JkyEjkbWQMQGavBARN8xo4avmuMh",
                                                 "DU4mqG99gi77BcZoS8FaJEiHk5HRYXNEcb",
                                                 "D6QdquB54saxViwAfL9xKiwoXaFo7UU5ec",
                                                 "DStTMUY2U1XSLsdq9uuWgfQefPDkQJkGQC",
                                                 "DPUV7Htc7jBwhc9z5rqoDFmKW3y1fE8xwA",
                                                 "DEGatQLqYCD9BumaXAqTFRCYZ4vznhQXcY",
                                                 "DCL7fkgzSSQSLvDMXqiRdnR9qQx5MjK89t",
                                                 "DNxc93Q2rrCm92sVyrtKhCn5MMC5YtuXxK",
                                                 "DDtWWXHe9a4EPn2EawiTDzYjq8SEKKRS6J",
                                                 "DNr54LSpN6iAQda1QYqykqeU7j7TyLeCcA",
                                                 "DE6eJePsjMDrTdKoi8HAGbX6Sdwh4RGTP9",
                                                 "D5kY1eMcDfLZWznQFSjCQMUW8DiSoxhmuy",
                                                 "D6dSnsPqcLaVvcH1MSFRMUy5KyVbnDufiX"};
}
#endif //DIGIASSET_CORE_DIGIASSETCONSTANTS_H
