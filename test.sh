tmux new-session -s "compute-${rack}" -n "compute-${rack}" -d
for i in 1 2; do
  tmux new-window -t "compute-${rack}:$i" -n "cpt-$i"
done

tmux select-window -t "compute-${rack}:1"


i=1
tmux send-keys -t :cpt-$i "cd ~/navit/navit/bin/navit/ && DISPLAY=:99 ./navit; exit" Enter
tmux join-pane -s :cpt-$i
tmux select-layout tiled

i=2
tmux send-keys -t :cpt-$i "sleep 5; pkill navit; tmux kill-session" Enter
tmux join-pane -s :cpt-$i
tmux select-layout tiled

#tmux set-window-option synchronize-panes
tmux -2 attach-session -t "compute-${rack}"
