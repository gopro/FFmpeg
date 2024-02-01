PRINT_INDENT = 16

print_dict: patches
	@(cd patches && for p in *; do printf "%*c{'name': '%s', 'level': 1},\n" $(PRINT_INDENT) " " $(basename $$p); done)
patches:
	git format-patch origin/master -o $@
clean:
	$(RM) -r patches

.PHONY: print_dict clean
