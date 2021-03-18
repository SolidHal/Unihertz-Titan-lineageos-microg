#!/bin/bash

# partitionmanager: support delayed adopted storage mount
repopick -g https://gerrit.twrp.me 3199
# partition: add support dm_use_original_path
repopick -g https://gerrit.twrp.me 3200
# cryptfs: add support for keymaster 2
repopick -g https://gerrit.twrp.me 3201
