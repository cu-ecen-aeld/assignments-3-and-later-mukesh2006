writefile=$1
writestr=$2


if [ $# -eq 2 ]; then
  if [ ! -d "$writefile" ]; then
    mkdir -p $(dirname "$writefile")
    touch "$writefile" | echo "$writestr" > "$writefile"
  else
    touch "$writefile" | echo "$writestr" > "$writefile"
  fi
else
  echo "Usage: ./writer.sh full_path_to_file string_to_write"
  exit 1
fi
