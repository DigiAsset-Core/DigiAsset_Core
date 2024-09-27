# Main:

```json
{
   "standard": 2,
   "contract": {
      "publisher": string,
      "address":   string
   },
   "alias": {  //optional include one of the following only 
        "domain":   "a domain that you own"
            // You must put a txt record in your domain name registry
            // Record name = _contractAddress.yourdomain  (please note some dns services may add your domain part automatically)
            // record value = "publishing address"
            //do not delete this record as future nodes will not be able to verify your alias

        "github":   "your github username"
            // You must create a repo called "social"
            // create a digibyte_smart_contracts.cfg that contains "contract address"="publishing address"
            //do not delete this repo or file as future nodes will not be able to verify your alias
    },
    "plugin": {
        "author": string,
        "path": should point to ipfs path "ipfs://cid" or "https://github.com/....",
        "name": string
    }
    "source": ["list of addresses assets can come from, must be at least 1"],
    "return": [see return section bellow],
    "trigger": {
        "notes": "a note explaining how contract works", //optional only required if execution may not be immediet",
        "expiry": block height or time in ms since epoch, //optional
        "maxexecutions": number of times it can be executed, //optional - not tracked by DigiAsset Core.   plugin maker should publish that it is disabled when count is reached.  Not tracked because plugin can determine what constitutes an execution
  //todo add getContractState RPC call so plugins can check instead of duplicating themselves
        "funds": [see funds section]
    }
}
```




# Return:
is an array of objects.  There are multiple formats the object can take.  You can mix any of the bellow together to make complicated outputs.  There must be at least one return value.  


Return an asset:
```json
{
    "assetId:assetIndex": count
}
```

Return more than 1 asset:
```json
{
    "assetId1:assetIndex": count,
    "assetId2:assetIndex": count
}
```

Return DigiByte:
```json
{
    "digibyte": sats
}
```

Return DigiByte and Assets:
```json
{
    "digibyte": sats,
    "assetId:assetIndex": count
}
```

Return DigiByte but based on fiat
```json
{
    "digibyte:address:index": sats,
}
```

Though it is generally not required to define what people other then the one triggering the contract will receives.  Some times(like time based or fund forwarding contracts) you may want to and can using sendto sections.  Probability value can not be used in sendto because these are already part of a main section that can have a probability.

```json
{
  "sendto:address": {
    return object
  },
  "sendto:assetId": {
    return object to be sent to all holders of an asset
  },
  "sendto:assetId:each": {
    return object to be sent to all holders(per unit held) of an asset
  },
  "sendto:assetId:assetIndex": {
    return object to be sent to all holders of a specific sub asset.
  },
  "sendto:assetId:assetIndex:each": {
    return object to be sent to all holders(per unit held) of a specific sub asset.
  }
}
```


Sometimes returned value is based on received value or amount in a specific payout pot address:

```json
{
   "balance:address": [numerator,denominator],
   "balance:received": [numerator,denominator]
}
```


### Probability:
If an output isn't guaranteed add.  
```json
"probability": count
```
odds will be count out of sum of counts.  So output is provably fair the getRandom RPC must be used and the returned value should be based on result. 
for example if we had probabilities of
5,10,100
and the result is 0 to 4 option 0 must be returned, 5 to 14 option 1, 15 and up option 2.
if there is a probability section with no assetId in it also then that probability section returns nothing.  
If there is a mix of probability and non probability sections then the sum of all non probability sections and the selected probability section must be paid out
for each probability section you can have an 
```json
"alternate": returnObject limited to just direct digibyte and assets(exchange rate is valid and so is nested alternate)
```
which contains an alternate id if any of the originals are not available.


A contract is considered disabled if chain data shows it can't pay out all possible outputs





# Funds:
same as send to but the following parameters are not allowed:
alternate, probability,sendto

if you want to do an auction you can use a fixed digibyte value and a reference to the balance in the trigger address.  you also need to add a note that it won't trigger immediately but at expiry or outbid.