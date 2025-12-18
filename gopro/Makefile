
VERSION      = 7.1
PRINT_INDENT = 16

# Print TOML blocks for each patch generated in ./patches
print_dict: patches
	@(cd patches && for p in *; do \
		printf '[[patches]]\nname = "%s"\nlevel = 1\n\n' "$$(basename $$p)"; \
	done)

# Generate patch files from the given git range into ./patches
patches:
	@FIRST=$$(git rev-list --ancestry-path --reverse origin/gopro-$(VERSION)..HEAD | head -n1); \
	if [ -z "$$FIRST" ]; then \
		echo "No commits after origin/gopro-$(VERSION) on the path to HEAD"; \
		exit 1; \
	fi; \
	echo "First commit after origin/gopro-$(VERSION): $$FIRST"; \
	# Create patches from (FIRST) through HEAD (i.e., starting right after base) \
	git format-patch $$FIRST..HEAD -o $@


## Remove the patches directory
clean:
	$(RM) -r patches

.PHONY: print_dict patches

