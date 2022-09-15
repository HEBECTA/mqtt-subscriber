#!/bin/sh

FILE=/log/mqtt_topics_data.db

if [ -f "$FILE" ]; then

        if [ -z "$1" ]; then

sqlite3 "$FILE" <<EOF
SELECT * FROM TOPICS_DATA;
EOF
        else

sqlite3 "$FILE" <<EOF
SELECT * FROM TOPICS_DATA WHERE Topic = "$1";
EOF

        fi

else

        echo "File not found !"

fi
