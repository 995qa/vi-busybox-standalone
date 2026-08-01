# BusyBox VI Standalone

A version of vi ripped straight from BusyBox 1.36.0 with vibe-coded header to make it work without Busybox.
Compiles fine for Linux and ravynOS 0.7.0 (macOS 10.15 with ravynOS SDK)

## Build for Linux GCC

```
gcc -O2 -Wall -I. -o vi vi.c main.c
```
for now until i decide (maybe) to make a Makefile for it
