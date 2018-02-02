---
layout: post
title:  "Putting something on the screen"
date:   2018-02-02 08:01:00 +0100
---

Rendering to the screen is handled by the Picture Processing Unit (PPU), the PPU has it's own internal memory called VRAM with an address space of 2^16 bytes (same as the CPU memory). Each frame, the PPU draws "whatever" is currently in the PPU memory. That's a pretty vague explanation but let's make it concrete by creating a sprite and draw it on the screen.

![Sprite]({{ "/assets/sprite.png" | absolute_url }})
The PPU memory can hold 64 sprites, each occupying 4 bytes
...

So, the pattern for the image above could look like this

```
0 0 0 0 0 0 0 1
0 0 0 0 0 0 1 1
0 0 0 0 0 1 1 1
0 0 0 0 1 1 1 1
0 0 0 1 1 1 1 3
0 0 1 1 1 1 1 3
0 1 1 1 1 1 3 3
1 1 1 1 3 3 3 3
```

and the corresponding colour palette:

```
0 => Transparent
1 => Green
2 => Don't care
3 => Red
```

Splitting the pattern into the two planes gives

```
$0xx0: 0 0 0 0 0 0 0 1   $0xx8: 0 0 0 0 0 0 0 0
$0xx1: 0 0 0 0 0 0 1 1   $0xx9: 0 0 0 0 0 0 0 0
$0xx2: 0 0 0 0 0 1 1 1   $0xxA: 0 0 0 0 0 0 0 0
$0xx3: 0 0 0 0 1 1 1 1   $0xxB: 0 0 0 0 0 0 0 0
$0xx4: 0 0 0 1 1 1 1 1   $0xxC: 0 0 0 0 0 0 0 1
$0xx5: 0 0 1 1 1 1 1 1   $0xxD: 0 0 0 0 0 0 0 1
$0xx6: 0 1 1 1 1 1 1 1   $0xxE: 0 0 0 0 0 0 1 1
$0xx7: 1 1 1 1 1 1 1 1   $0xxF: 0 0 0 0 1 1 1 1
``` 
