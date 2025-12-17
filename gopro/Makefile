
VERSION      = 7.1
PRINT_INDENT = 16

# Print TOML blocks for each patch generated in ./patches
print_dict: patches
	@(cd patches && for p in *; do \
		printf '[[patches]]\nname = "%s"\nlevel = 1\n\n' "$$(basename $$p)"; \
	done)

# Generate patch files from the given git range into ./patches
patches:
	git format-patch origin/gopro-$(VERSION)..HEAD^ -o $@

## Remove the patches directory
clean:
	$(RM) -r patches

.PHONY: print_dict patches

