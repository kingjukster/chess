# Syzygy Tablebase Support

This chess engine supports Syzygy endgame tablebases for perfect endgame play.

## What are Tablebases?

Tablebases are endgame databases that contain perfect play information for all positions with a limited number of pieces. They provide:

- **Perfect play**: Guaranteed optimal moves in endgame positions
- **WDL information**: Win/Draw/Loss evaluation
- **DTZ information**: Distance to zeroing move (capture or pawn move)
- **Mate distances**: Exact moves to checkmate

## Syzygy Format

The engine uses Syzygy tablebases via the Fathom library:

- **Compressed format**: Much smaller than previous formats (Nalimov, Gaviota)
- **Fast probing**: Optimized for speed
- **WDL + DTZ tables**: Separate tables for evaluation and optimal play
- **Up to 7 pieces**: Commonly available (3-7 piece endgames)

### File Types

Syzygy tablebases come in two types:

1. **WDL files** (`.rtbw`): Win/Draw/Loss information
   - Required for evaluation
   - Smaller files
   - Example: `KQvKR.rtbw`

2. **DTZ files** (`.rtbz`): Distance to zeroing move
   - Optional but recommended
   - Larger files
   - Needed for optimal 50-move rule play
   - Example: `KQvKR.rtbz`

## Obtaining Tablebases

### Download Locations

Official Syzygy tablebase downloads:

1. **3-4-5 piece** (~1 GB total)
   - Essential endgames
   - Download: https://syzygy-tables.info/
   - Includes: KQK, KRK, KPK, KQKQ, KRKR, etc.

2. **6-piece** (~150 GB)
   - Extended coverage
   - Download: https://syzygy-tables.info/
   - Torrent recommended

3. **7-piece** (~16 TB)
   - Complete coverage
   - Download: https://syzygy-tables.info/
   - Requires significant storage

### Recommended Setup

For most users:
```
tablebases/
├── 3-4-5/          # Essential (1 GB)
│   ├── KQvK.rtbw
│   ├── KQvK.rtbz
│   ├── KRvK.rtbw
│   ├── KRvK.rtbz
│   └── ...
└── 6/              # Optional (150 GB)
    ├── KQQvKQ.rtbw
    └── ...
```

### Download Methods

#### Direct Download
```bash
# Download 3-4-5 piece tables
wget -r -np -nH --cut-dirs=1 https://syzygy-tables.info/download/3-4-5/

# Or use curl
curl -O https://syzygy-tables.info/download/3-4-5/[file].rtbw
```

#### Torrent (Recommended for 6-7 piece)
```bash
# Use BitTorrent client
# Torrent files available at: https://syzygy-tables.info/
```

## Using Tablebases

### UCI Configuration

Configure tablebases through UCI options:

```
# Set tablebase path (can contain multiple directories separated by ;)
setoption name SyzygyPath value /path/to/tablebases

# Set probe depth (minimum depth to probe, default: 1)
setoption name SyzygyProbeDepth value 1

# Set probe limit (maximum pieces to probe, default: 7)
setoption name SyzygyProbeLimit value 7
```

### Arena/ChessBase Setup

1. **Arena Chess GUI**:
   - Engines → Manage → Select engine
   - General → Configure
   - Set "SyzygyPath" to your tablebase directory
   - Adjust SyzygyProbeDepth and SyzygyProbeLimit

2. **ChessBase/Fritz**:
   - Engine → Configure
   - Tablebases tab
   - Add Syzygy path
   - Enable "Use tablebases"

### Command Line

```bash
# Start engine
./chess_engine

# Configure tablebases
setoption name SyzygyPath value /home/user/tablebases/3-4-5

# Verify loading
# Engine will output: "info string Syzygy tablebases loaded: /home/user/tablebases/3-4-5"
# And: "info string Max cardinality: 5"

# Test with endgame position
position fen 8/8/8/4k3/8/8/2Q5/4K3 w - - 0 1
go depth 10

# Engine will output: "info string Tablebase hit - WDL: 4 DTZ: 10"
```

## Tablebase Settings Explained

### SyzygyPath (string, default: empty)

Path to tablebase files:
- **Single directory**: `/path/to/tablebases`
- **Multiple directories**: `/path/3-4-5;/path/6` (semicolon separated)
- **Relative path**: `./tablebases` (relative to engine)
- **Empty**: Tablebases disabled

### SyzygyProbeDepth (1-100, default: 1)

Minimum search depth before probing:
- **1**: Probe at all depths (recommended)
- **Higher**: Only probe in deeper searches
- **Use case**: Reduce probing overhead in very fast games

### SyzygyProbeLimit (3-7, default: 7)

Maximum number of pieces to probe:
- **3**: Only KvK, KQvK, KRvK, etc.
- **5**: Up to 5-piece endgames (recommended minimum)
- **6**: Requires 6-piece tables (~150 GB)
- **7**: Requires 7-piece tables (~16 TB)

**Important**: Set this to match your available tablebases!

## How Tablebases are Used

### 1. Root Position Probing

When `go` command is issued, engine checks if position is in tablebase:

```
position fen 8/8/8/4k3/8/8/2Q5/4K3 w - - 0 1
go depth 10

# Output:
info string Tablebase hit - WDL: 4 DTZ: 10
bestmove c2c5
```

If position is in tablebase, engine immediately returns the optimal move without searching.

### 2. Search Tree Probing

During search, when reaching tablebase positions:

```cpp
// In alpha-beta search
if (tablebase && depth >= probe_depth) {
    int tb_score;
    if (tablebase->probe_search(pos, tb_score)) {
        return tb_score;  // Use perfect evaluation
    }
}
```

This provides perfect evaluation for endgame positions during search.

### 3. WDL vs DTZ

**WDL (Win/Draw/Loss)**: Used during search
- TB_WIN (4): Position is winning
- TB_CURSED_WIN (3): Winning but 50-move rule draw possible
- TB_DRAW (2): Position is drawn
- TB_BLESSED_LOSS (1): Losing but 50-move rule draw possible
- TB_LOSS (0): Position is losing

**DTZ (Distance to Zeroing)**: Used at root
- Exact number of moves to next capture/pawn move
- Used to find optimal move respecting 50-move rule
- Ensures shortest path to conversion

## Performance

Tablebase probing is very fast:
- **Probe time**: < 1ms typical
- **Cache**: Recently probed positions cached
- **Overhead**: Minimal in non-endgame positions

### Statistics

Check tablebase usage:
```
# After search, engine tracks:
info string TB hits: 1234
```

## Troubleshooting

### Tablebases not loading

```
info string Failed to load Syzygy tablebases: /path/to/tb
```

**Solutions**:
- Verify path exists and contains `.rtbw` files
- Check file permissions (must be readable)
- Ensure path format is correct (no trailing slash)
- Try absolute path instead of relative

### No tablebase hits

**Causes**:
- Position has too many pieces (> SyzygyProbeLimit)
- Required tablebase file missing
- Only WDL files present (need DTZ for root probing)
- Position not in tablebase (e.g., 8 pieces)

**Solutions**:
- Download missing tablebase files
- Reduce SyzygyProbeLimit to match available tables
- Download DTZ files (`.rtbz`) in addition to WDL

### Incorrect moves in endgame

**Causes**:
- Missing DTZ files (only WDL present)
- 50-move rule not properly handled
- Tablebase files corrupted

**Solutions**:
- Download complete tablebase sets (both WDL and DTZ)
- Verify file integrity (checksums)
- Re-download corrupted files

## Verification

Test tablebase functionality with known positions:

### KQ vs K (White to move, mate in 10)
```
position fen 8/8/8/4k3/8/8/2Q5/4K3 w - - 0 1
go depth 1

# Expected:
info string Tablebase hit - WDL: 4 DTZ: 10
bestmove c2c5  # or another optimal move
```

### KR vs K (White to move, mate in 16)
```
position fen 8/8/8/4k3/8/8/2R5/4K3 w - - 0 1
go depth 1

# Expected:
info string Tablebase hit - WDL: 4 DTZ: 16
bestmove c2c4  # or another optimal move
```

### K vs K (Draw)
```
position fen 8/8/8/4k3/8/8/8/4K3 w - - 0 1
go depth 1

# Expected:
info string Tablebase hit - WDL: 2 DTZ: 0
bestmove e1e2  # any legal move
```

## Storage Requirements

Approximate sizes:

| Pieces | WDL Only | WDL + DTZ | Positions |
|--------|----------|-----------|-----------|
| 3      | 1 MB     | 2 MB      | ~4K       |
| 4      | 50 MB    | 100 MB    | ~1M       |
| 5      | 500 MB   | 1 GB      | ~100M     |
| 6      | 70 GB    | 150 GB    | ~10B      |
| 7      | 8 TB     | 16 TB     | ~1T       |

**Recommendation**: Start with 3-4-5 piece (1 GB), add 6-piece if storage allows.

## Advanced Usage

### Multiple Tablebase Paths

Separate different piece counts:
```
setoption name SyzygyPath value /ssd/3-4-5;/hdd/6;/nas/7
```

Benefits:
- Fast storage (SSD) for frequently accessed tables
- Slower storage (HDD/NAS) for rarely used tables

### Probe Depth Optimization

For ultra-fast games (bullet/blitz):
```
setoption name SyzygyProbeDepth value 3
```

Reduces probing overhead by only checking in deeper searches.

### Selective Piece Counts

If you only have 3-4-5 piece tables:
```
setoption name SyzygyProbeLimit value 5
```

Prevents attempting to probe unavailable 6-7 piece positions.

## Technical Details

### Fathom Library

This engine uses the Fathom library for Syzygy probing:
- **Source**: https://github.com/jdart1/Fathom
- **License**: MIT
- **Features**: Fast, accurate, well-tested
- **Integration**: Included in `tablebase/fathom/`

### Zobrist Hashing

Syzygy uses a different zobrist hashing scheme than the engine's internal hash. The tablebase module handles conversion automatically.

### 50-Move Rule

DTZ tables account for the 50-move rule:
- **Cursed wins**: Positions that are winning but can be drawn by 50-move rule
- **Blessed losses**: Positions that are losing but can be drawn by 50-move rule

Engine respects these evaluations when probing.

## Best Practices

1. **Start small**: Begin with 3-4-5 piece tables
2. **Verify loading**: Check engine output confirms tablebase load
3. **Test functionality**: Use known endgame positions
4. **Match probe limit**: Set SyzygyProbeLimit to your available tables
5. **Use SSD**: Store frequently used tables on fast storage
6. **Download both**: Get both WDL and DTZ files for complete functionality

## See Also

- [Opening Books](OPENING_BOOKS.md) - Opening book support
- [UCI Protocol](UCI.md) - Full UCI options reference
- [Syzygy Website](https://syzygy-tables.info/) - Official tablebase source
- [Fathom Library](https://github.com/jdart1/Fathom) - Probing library
