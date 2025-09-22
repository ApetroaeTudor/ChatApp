### Chat App

avahi-browse -r -t _ChatAppServer._tcp | awk -F'[][]' '/address/ && $2 ~ /^192\.168\./ {print $2}'