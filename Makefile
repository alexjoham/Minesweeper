.PHONY: minesweeper

minesweeper:
	rm -f minesweeper
	clang++ -std=c++17 -O2 -I. main.cpp game/game.cpp button/button.cpp -o minesweeper
