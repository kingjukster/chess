# Testing Your Engine

## Quick Test

The search at depth 6 might be slow. Try a lower depth first:

```
uci
setoption name Use NNUE value false
isready
position startpos
go depth 3
```

You should see output like:
```
info depth 1 nodes 20 score cp 25 pv e2e4
info depth 2 nodes 400 score cp 15 pv e2e4
info depth 3 nodes 8902 score cp 20 pv e2e4
bestmove e2e4
```

## If Search Hangs

If the search seems to hang:

1. **Try depth 1-2 first** - These should be very fast
2. **Check CPU usage** - The search might just be slow (depth 6 can take minutes)
3. **Use ClassicEval** - NNUE might be slower: `setoption name Use NNUE value false`
4. **Add timeout** - Use time controls instead: `go wtime 5000 btime 5000`

## Expected Performance

- Depth 1: < 1 second
- Depth 2: < 1 second  
- Depth 3: 1-5 seconds
- Depth 4: 5-30 seconds
- Depth 5: 30 seconds - 2 minutes
- Depth 6: 2-10 minutes (depends on position)

## Debugging

If search never completes:

1. Check for infinite loops in quiescence
2. Verify move generation is correct (use perft)
3. Check evaluation doesn't crash
4. Try with ClassicEval first to isolate NNUE issues

## Perft Test (Fast)

Verify move generation works:
```
uci
perft 1
perft 2
perft 3
```

Expected: 20, 400, 8902

