
SOURCE:=$(shell find modules)

build/lib/doc/robots.txt: $(SOURCE)
	npx antora --fetch local-playbook.yml

.PHONY: build
build : build/lib/doc/robots.txt
