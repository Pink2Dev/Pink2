
Pinkcoin 2 development tree
===========================

[![CircleCI](https://circleci.com/gh/Pink2Dev/Pink2.svg?style=svg)](https://circleci.com/gh/Pink2Dev/Pink2)

What is Pinkcoin?
-----------------

Pinkcoin is a cryptocurrency using hybrid Proof of Work and Proof of Stake consensus protocols designed to support Charities and Non-Profit organizations, and encourage community participation. Proof of Stake consensus is split into two unique parts:
- Proof of Stake 3.0 
- [Flash Proof of Stake 2.0](https://pinkcoin.gitbook.io/pinkcoin/faq/fpos-2.0)

Additional unique features:
- [Side staking mechanism](https://pinkcoin.gitbook.io/pinkcoin/guides/side-staking)

Coin specification
-------------

### Misc data ###

| | |
| :---   |      ---: |
| PoW algorithm | Scrypt |
| Difficulty | Retargeting every block |
| Maximum coin supply | 500,000,000 PINK |

### Block creation ###

|        | PoW | PoS | FPoS |
| :---   |---: |---: |---:  |
| Block time  | 120s | 360s | 60s |
| When produced | 24h / day | 20h / day | 4h / day |


### Minting rewards ###

Halving of blocks rewards will occur every 846800 blocks so for example:

| Block number | PoW reward | PoS reward | FPoS reward |
|---:|---:|---:|---:|
| <   846,800 | 50 PINK | 100 PINK | 150 PINK |
| < 1,693,600 | 25 PINK | 50 PINK | 75 PINK |

and so on.

Development process
-------------------

Developers work in their own trees, then submit pull requests when
they think their feature or bug fix is ready.

The patch will be accepted if there is broad consensus that it is a
good thing.  Developers should expect to rework and resubmit patches
if they don't match the project's coding conventions (see coding.txt)
or are controversial.

The master branch is regularly built and tested, but is not guaranteed
to be completely stable. Tags are regularly created to indicate new
stable release versions of Pinkcoin.

Feature branches are created when there are major new features being
worked on by several people.

From time to time a pull request will become outdated. If this occurs, and
the pull is no longer automatically mergeable; a comment on the pull will
be used to issue a warning of closure. The pull will be closed 15 days
after the warning if action is not taken by the author. Pull requests closed
in this manner will have their corresponding issue labeled 'stagnant'.

Issues with no commits will be given a similar warning, and closed after
15 days from their last activity. Issues closed in this manner will be 
labeled 'stale'.
