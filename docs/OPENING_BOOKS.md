# Opening Book Support

This chess engine supports Polyglot format opening books for improved opening play.

## What is an Opening Book?

An opening book is a database of chess opening moves that allows the engine to play strong, well-known openings without having to search. This saves time and ensures the engine plays established theory in the opening phase.

## Polyglot Format

The engine uses the Polyglot book format, which is:
- **Binary format**: Compact and fast to probe
- **Industry standard**: Compatible with many chess engines
- **Weighted entries**: Moves can be weighted by frequency or success rate
- **Sorted by position hash**: Enables fast binary search

### File Structure

Each entry in a Polyglot book is 16 bytes:
- 8 bytes: Position hash (zobrist key)
- 2 bytes: Move (from/to/promotion encoded)
- 2 bytes: Weight (higher = more likely to be played)
- 4 bytes: Learn data (unused by this engine)

## Obtaining Opening Books

### Download Pre-made Books

Several high-quality Polyglot books are available:

1. **Performance.bin** - Strong GM games
   - Download: http://www.rivalchess.com/books/
   - Size: ~50MB
   - Depth: ~20 ply

2. **Human.bin** - Human master games
   - Download: http://www.rivalchess.com/books/
   - Size: ~30MB
   - Variety: High

3. **Computer.bin** - Computer games
   - Download: http://www.rivalchess.com/books/
   - Size: ~40MB
   - Accuracy: Very high

4. **Cerebellum** - Large comprehensive book
   - Download: https://zipproth.de/Brainfish/download/
   - Size: ~800MB
   - Coverage: Extensive

### Generate Your Own Book

Use the included `make_book.py` tool to generate books from PGN files:

```bash
# Basic usage
python tools/make_book.py games.pgn mybook.bin

# Filter by rating
python tools/make_book.py games.pgn mybook.bin --min-rating 2500

# Filter by result (only winning games for white)
python tools/make_book.py games.pgn mybook.bin --result "1-0"

# Filter by opening
python tools/make_book.py games.pgn mybook.bin --opening "Sicilian"

# Weight by success rate instead of frequency
python tools/make_book.py games.pgn mybook.bin --weight-by-result

# Limit depth and require minimum games
python tools/make_book.py games.pgn mybook.bin --max-ply 30 --min-games 5
```

#### Requirements

The book generator requires Python 3.6+ and python-chess:

```bash
pip install python-chess
```

#### PGN Sources

Download PGN game collections from:
- **TWIC** (The Week in Chess): https://theweekinchess.com/twic
- **Lichess Database**: https://database.lichess.org/
- **CCRL Games**: http://www.computerchess.org.uk/ccrl/
- **CEGT Games**: http://www.cegt.net/

## Using Opening Books

### UCI Configuration

Configure the opening book through UCI options:

```
# Enable opening book
setoption name OwnBook value true

# Set book file path
setoption name BookFile value /path/to/book.bin

# Set maximum book depth (ply)
setoption name BookDepth value 20

# Set variety (0-100, higher = more variety)
setoption name BookVariety value 50

# Always play best move (ignore variety)
setoption name BookBestMove value false
```

### Arena/ChessBase Setup

1. **Arena Chess GUI**:
   - Engines → Manage → Select engine
   - General → Configure
   - Set "OwnBook" to true
   - Set "BookFile" to your book path
   - Adjust BookDepth and BookVariety as desired

2. **ChessBase/Fritz**:
   - Engine → Configure
   - Books tab
   - Add Polyglot book
   - Enable "Use opening book"

### Command Line

```bash
# Start engine
./chess_engine

# Configure book
setoption name OwnBook value true
setoption name BookFile value performance.bin
setoption name BookDepth value 20
setoption name BookVariety value 50

# Play
position startpos
go depth 10
```

## Book Settings Explained

### BookDepth (1-100, default: 20)

Maximum ply (half-moves) to use the book:
- **Lower (10-15)**: Exit book early, more engine analysis
- **Higher (25-40)**: Stay in book longer, follow theory deeper
- **Recommended**: 20 for balanced play

### BookVariety (0-100, default: 50)

Controls move selection randomness:
- **0**: Always play highest weighted move (deterministic)
- **50**: Balanced - weights matter but variety exists
- **100**: Uniform random selection (all moves equal chance)
- **Recommended**: 
  - 0-20 for maximum strength
  - 40-60 for variety in training/analysis
  - 80-100 for maximum unpredictability

### BookBestMove (true/false, default: false)

- **false**: Use weighted random selection (respects BookVariety)
- **true**: Always play highest weighted move (ignores BookVariety)
- **Recommended**: false for most use cases

## Book Move Selection Algorithm

The engine uses weighted random selection:

1. **Probe book**: Find all moves for current position
2. **Filter**: Remove moves below minimum weight threshold
3. **Adjust weights**: Scale based on BookVariety setting
4. **Select**: Random selection weighted by adjusted weights

### Example

Position has 3 book moves:
- e4: weight 1000 (played 1000 times)
- d4: weight 500 (played 500 times)
- Nf3: weight 100 (played 100 times)

With BookVariety=0:
- e4: 100% chance (always best)

With BookVariety=50:
- e4: ~62% chance
- d4: ~31% chance
- Nf3: ~7% chance

With BookVariety=100:
- e4: 33% chance (uniform)
- d4: 33% chance
- Nf3: 33% chance

## Performance

Opening book probing is very fast:
- **Lookup time**: < 1ms typical
- **Memory**: Book file is memory-mapped (no RAM overhead)
- **Disk I/O**: Minimal due to binary search

## Troubleshooting

### Book not loading

```
info string Failed to load opening book: book.bin
```

**Solutions**:
- Check file path is correct (absolute or relative to engine)
- Verify file exists and is readable
- Ensure file is valid Polyglot format
- Check file permissions

### No book moves

```
info string Book move
bestmove (none)
```

**Causes**:
- Position not in book (exited book depth)
- All moves filtered by minimum weight
- Book depth limit reached
- Book doesn't cover this line

**Solutions**:
- Use larger/deeper book
- Reduce minimum weight threshold
- Increase BookDepth setting

### Book moves seem weak

**Solutions**:
- Use higher quality book (GM games, higher rating filter)
- Reduce BookVariety (play stronger moves more often)
- Enable BookBestMove (always play best)
- Generate custom book with `--weight-by-result`

## Best Practices

1. **Match book to playing style**:
   - Aggressive: Books from 1-0 games only
   - Solid: Books from high-rated draws
   - Balanced: Mixed results, high rating

2. **Depth vs. Coverage tradeoff**:
   - Deeper books (30+ ply) = narrow but deep
   - Wider books (15-20 ply) = many lines, less depth
   - Use multiple books for different situations

3. **Testing**:
   - Test book with `d` command to see if moves are from book
   - Compare engine performance with/without book
   - Adjust variety based on results

4. **Updates**:
   - Regenerate books periodically with new games
   - Update from latest TWIC/Lichess databases
   - Filter by recent years for modern theory

## Advanced: Book Statistics

To analyze a book's contents, use the book generator in analysis mode:

```python
# Analyze book statistics
python tools/make_book.py --analyze book.bin
```

This shows:
- Total positions
- Average moves per position
- Depth distribution
- Weight distribution
- Most common openings

## See Also

- [Syzygy Tablebases](TABLEBASES.md) - Endgame tablebases
- [UCI Protocol](UCI.md) - Full UCI options reference
- [Polyglot Specification](http://hgm.nubati.net/book_format.html) - Format details
