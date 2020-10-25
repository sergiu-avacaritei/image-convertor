default: converter

converter: converter.c
	clang -std=c11 -Wall -pedantic -g converter.c -o $@ \
	    -fsanitize=undefined -fsanitize=address

%: %.c
	clang -Dtest_$@ -std=c11 -Wall -pedantic -g $@.c -o $@ \
	    -fsanitize=undefined -fsanitize=address
