default:
	gcc -o tictac tictac.c
optimal:
	gcc -O3 -o optimized tictac.c
debug:
	gcc -g -o tictac tictac.c
