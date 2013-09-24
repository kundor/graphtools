
SOURCES = planar/stats_pl.c planar/count_pl.c planar/filter_pl.c\
          planar/split_pl.c planar/dual_pl.c planar/non_iso_pl/non_iso_pl.c\
          planar/non_iso_pl/hashfunction.c planar/non_iso_pl/splay.c\
	  planar/subdivide_vertex.c planar/regular_pl.c\
          conversion/gconv.c conversion/gconvman.txt conversion/Makefile\
          Makefile COPYRIGHT.txt LICENSE.txt README.md

all: build/stats_pl build/count_pl build/filter_pl \
	build/split_pl build/nauty_pl build/dual_pl \
	build/non_iso_pl build/gconv build/subdivide_vertex \
	build/regular_pl

clean:
	rm -rf build
	rm -rf dist

build/stats_pl: planar/stats_pl.c
	mkdir -p build
	cc -o build/stats_pl -O4 planar/stats_pl.c

build/count_pl: planar/count_pl.c
	mkdir -p build
	cc -o build/count_pl -O4 planar/count_pl.c

build/filter_pl: planar/filter_pl.c
	mkdir -p build
	cc -o build/filter_pl -O4 planar/filter_pl.c

build/split_pl: planar/split_pl.c
	mkdir -p build
	cc -o build/split_pl -O4 planar/split_pl.c

build/nauty_pl: planar/nauty_pl.c planar/nauty/nauty.c planar/nauty/nautil.c planar/nauty/naugraph.c planar/nauty/schreier.c planar/nauty/naurng.c
	mkdir -p build
	cc -o build/nauty_pl -O4 planar/nauty_pl.c planar/nauty/nauty.c \
	planar/nauty/nautil.c planar/nauty/naugraph.c planar/nauty/schreier.c \
	planar/nauty/naurng.c
	
build/dual_pl: planar/dual_pl.c
	mkdir -p build
	cc -o build/dual_pl -O4 planar/dual_pl.c
	
build/regular_pl: planar/regular_pl.c
	mkdir -p build
	cc -o build/regular_pl -O4 planar/regular_pl.c

build/non_iso_pl: planar/non_iso_pl/non_iso_pl.c planar/non_iso_pl/hashfunction.c planar/non_iso_pl/splay.c
	mkdir -p build
	cc -o build/non_iso_pl -O4 planar/non_iso_pl/non_iso_pl.c
	
build/subdivide_vertex: planar/subdivide_vertex.c
	mkdir -p build
	cc -o build/subdivide_vertex -O4 planar/subdivide_vertex.c

build/gconv: conversion/gconv.c
	cd conversion && make

sources: dist/graphtools-sources.zip dist/graphtools-sources.tar.gz

dist/graphtools-sources.zip: $(SOURCES)
	mkdir -p dist
	zip dist/graphtools-sources $(SOURCES)

dist/graphtools-sources.tar.gz: $(SOURCES)
	mkdir -p dist
	tar czf dist/graphtools-sources.tar.gz $(SOURCES)
