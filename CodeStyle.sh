HOST=$(uname -s)
../tools/AStyle/bin/astyle_$HOST --options=astyle.opt -n  Replay_Boot/*.h Replay_Boot/*.c
