SHELL := /bin/bash

CC := gcc
CFLAGS := -Wall -Werror -Wextra -x c -std=c11 -pedantic -c
LDFLAGS := -lcheck -lsubunit -lpthread

project_name := tetris
target_dir := .
obj_dir := ${target_dir}/obj
bin_dir := ${target_dir}/bin
source_dir := ./${project_name}
test_dir := ${target_dir}/test
doc_dir := ${target_dir}/doc

find_source = find $(1) -iname "*.c"
find_header = find $(1) -iname "*.h"
find_gcov = find $(1) -iname "*.gcda" -o -iname '*.gcno'

lib_name := $(addsuffix .a, $(project_name))
test_lib_name := $(addsuffix _test.a, $(project_name))

sources := $(shell $(call find_source, $(source_dir)))
headers := $(shell $(call find_header, $(source_dir)))
test_sources := $(shell $(call find_source, $(test_dir)))

objects := $(patsubst %.c, %.o, $(sources))
gcov_files := $(shell $(call find_gcov, $(source_dir)))

cstyle := clang-format
cstyle_nflag := -n
cstyle_iflag := -i
ccheck := cppcheck
ccheck_flags := --enable=all --force --suppress=missingIncludeSystem --language=c --std=c11

.PHONY: all build install dvi uninstall clean styletest clangi

all: build dvi

build: $(lib_name) clean_obj

install:
	mkdir -p

uninstall:

dvi:
	doxygen Doxyfile

${obj_dir}:
	mkdir -p $@

$(bin_dir):
	mkdir -p $@

%.o: %.c
	$(CC) $(CFLAGS) -o $(addprefix $(obj_dir)/, $(notdir $@)) $<

$(lib_name): $(bin_dir) $(obj_dir) $(objects)
	@ar rc $(addprefix $(bin_dir)/, $(notdir $@)) $(addprefix ${obj_dir}/, $(notdir ${objects}))
	@ranlib $(addprefix $(bin_dir)/, $(notdir $@))

clean_obj: $(objects)
	rm -f $(addprefix $(obj_dir)/, $(notdir $@))
	rm -rf $(obj_dir)

clean:
	rm -f ${objects}
	rm -f ${gcov_files}
	rm -f ${lib_name}
	rm -f ${test_lib_name}
	rm -rf ${built_dir} ${obj_dir} ${bin_dir}
	rm -rf ${doc_dir}

styletest: $(cstyle) $(ccheck)

$(ccheck): $(sources) $(headers) $(test_surces)
	$@ $(ccheck_flags) $^

$(cstyle): $(sources) $(headers) $(test_sources)
	cp ../../materials/linters/.clang-format ${target_dir}
	$@ ${cstyle_nflag} $^
	rm -f ./.clang-format

clangi: $(sources) $(headers) $(test_sources)
	cp ../../materials/linters/.clang-format ${target_dir}
	$(cstyle) ${cstyle_iflag} $^
	rm -f ./.clang-format