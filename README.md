# 2048 for Q2 Pod

A native card app for the Shanling Q2, based on Gabriele Cirulli's 2048. Join matching tiles to reach 2048, then choose **Keep playing** to go higher.

## Install

1. Extract `q2-2048.zip` into the root of the SD card. The extracted paths should include `.2048/q2app` and `.2048/q2-2048`. Your file browser may hide the folder because its name begins with a dot.
2. Eject the card and open **Library → Games → 2048**. You need a version of [Q2 Pod](https://github.com/DiamondBond/q2-pod) that supports full-screen card games such as Buddy.

| Control | Action |
| --- | --- |
| Swipe across the board | Move in the swipe direction |
| Tap a direction button | Move in that direction |
| Turn the clickwheel | Select a direction or menu item |
| Press Centre | Move in the selected direction or choose a menu item |
| Press Return | Open the pause menu during play |
| Hold Return | Close the app through Q2 Pod |

The current board and best score are saved to `.2048/state` after a move. A finished board starts a new game on the next launch and keeps your best score.

## Build

From the repository root, use Clang, LLD, Make, and zip:

```sh
make
make check
```

The build produces `dist/q2-2048.zip`. The binary uses the Q2's MIPS32r2, little-endian, NaN2008 ABI and needs no stock firmware or cross sysroot to build. The checks run on your computer with its C compiler.

The 16-byte board stores tile exponents, following [2048.c](https://github.com/mevdschee/2048.c).

The Q2 launch, framebuffer, and input socket follow [Buddy](https://github.com/DiamondBond/q2-fidget). The Liberation Sans Bold bitmap comes from [GrooveBox](https://github.com/DiamondBond/q2-groovebox). See [LICENSE.txt](LICENSE.txt) for the game, Buddy code, and font notices.

## Automatic builds and releases

GitHub Actions tests and builds each push to `main` and every pull request. Download the **q2-2048-card-app** artifact from a successful run, or start a build with **Actions → q2-2048 → Run workflow**.

Push an unused version tag to publish a release automatically:

```sh
git pull --ff-only
git tag v1.0.1
git push origin v1.0.1
```

Once the build and package checks pass, Actions creates the release and attaches **q2-2048.zip** directly. Re-running the tag build replaces that release's ZIP. Branch pushes and manual branch builds only save Actions artifacts.
