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

The Q2 launch, framebuffer, and input socket follow [Buddy](https://github.com/DiamondBond/q2-fidget). The Liberation Sans Bold bitmap comes from [GrooveBox](https://github.com/DiamondBond/q2-groovebox). See the original game's [MIT license](LICENSE.txt), [Buddy license](LICENSE-BUDDY.txt), and [font license](LICENSE-FONT.txt).
