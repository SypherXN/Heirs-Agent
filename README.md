# Heirs Game Agent

This project is a high-performance C++ game-playing agent developed for the Heirs assignment. The agent is built around a depth-limited minimax search with strong pruning and ordering techniques, designed to balance decision quality with strict time constraints.

## Overview

The agent focuses on reaching deeper search depths while maintaining strong move quality. Over time, the design has shifted away from heavy evaluation heuristics toward efficient search improvements, after observing that excessive evaluation logic reduced performance and overall strength.

The current approach emphasizes:

- Fast search and high node throughput  
- Strong move ordering and pruning  
- Lightweight evaluation with targeted heuristics  
- Careful time management  

## Development Process

This project was developed with the assistance of ChatGPT, which was used for both planning and implementation support. The focus was on iterating quickly, testing ideas, and refining approaches based on performance impact rather than adding complexity for its own sake.

## Core Features

### Search

- Minimax with alpha-beta pruning  
- Principal Variation Search  
- Iterative deepening  
- Aspiration windows at the root  
- Quiescence search  
- Late Move Reductions with gating conditions  
- Selective extensions with tighter control  
- Root-level guard for prince race scenarios  
- Hard depth cap  

### Time Management

- Time-based depth selection  
- Opponent-time-aware adjustments  
- Additional depth when ahead on time  
- Pre-iteration affordability check  
- Early termination on timeout  

### Move Ordering

- MVV-style capture prioritization  
- Static Exchange Evaluation for capture decisions  
- Transposition table best move ordering  
- Killer heuristic  
- History heuristic  
- Countermove heuristic  
- Lightweight partial ordering  

### Evaluation

- Incremental material evaluation  
- Terminal detection via Prince capture  
- Ply-relative mate scoring  
- Mate score normalization for consistency  
- Prince safety heuristics:
  - Penalty when own Prince is threatened  
  - Reward when opponent Prince is threatened  
- Draw-aware scoring based on:
  - Time advantage  
  - Remaining material  

### Draw Handling

- Repetition detection using hash history  
- Persistent state stored across moves  
- 50-move rule tracking  
- Draws treated as terminal states with context-aware scoring  

### Performance Optimizations

- Incremental Zobrist hashing  
- Transposition table with exact, upper, and lower bounds  
- Fixed-size arrays to avoid dynamic allocation  
- Efficient make and unmake operations  
- Piece lists for constant-time updates  
- Lightweight time checks  

## Design Philosophy

A key takeaway from development is that more evaluation detail does not always lead to better play. Adding many heuristics significantly slowed the search, reducing depth and weakening overall performance.

Because of this, the agent prioritizes:

- Maintaining high search speed  
- Avoiding expensive evaluation logic  
- Introducing only high-impact, low-cost improvements  

Some approaches were intentionally avoided or deprioritized after testing, including heavy repetition logic tuning and attack map construction, due to limited benefit relative to their cost.

## Structure

The implementation is contained in a single C++ source file:

- `homework.cpp`

The code is organized into clearly separated sections with comments, and all tunable parameters are grouped in a configuration struct at the top of the file.

## Notes

This agent is designed for a competitive, time-constrained environment where both speed and accuracy matter. Most improvements are evaluated based on their effect on depth and practical playing strength rather than theoretical strength alone.

Future work should continue focusing on improvements that increase decision quality without sacrificing performance.
