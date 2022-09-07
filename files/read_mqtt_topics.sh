#!/bin/sh
sqlite3 /log/mqtt_topics_data.db <<EOF
select * from TOPICS_DATA;
EOF