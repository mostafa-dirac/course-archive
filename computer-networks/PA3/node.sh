source info.sh
if [ "a$1" = "a" ]; then
  node="node0"
else
  node="node$1"
fi
./node.out --ip $ip --port $port --map $map --node $node --user $user --pass "$pass" --id $user
