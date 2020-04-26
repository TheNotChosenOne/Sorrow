# Sorrow Engine

A 2d, entity-component oriented, data oriented, game engine, with type safety using template metaprogramming. For Linux.

## Requirements

Requires the following packages:
* `libbox2d-dev`
* `libcgal-dev`
* `libglew-dev`
* `libsdl2-dev`
* `coz-profiler`
* `valgrind`

You also need to have repository `https://github.com/Manu343726/ctti` cloned in the parent repository,
so your directories should look like:
```
--|-- ctti -- ...
  |-- Sorrow <- this directory
```

Alternatively, you can put it anywhere else but you'll need to update the Makefile

## Compiling

Run `make`
