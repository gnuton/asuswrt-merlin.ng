#!/usr/bin/python3
import re
from datetime import date
import os

today=date.today()
CHANGELOG_FILE='Changelog-386.txt'
MSG="""##  GNUton's Asus Merlin changelog  ##\n"""
TAGGED_RELEASE_ENV_NAME='CIRCLE_TAG'

latest_release = None
latest_changes = list()

with open(CHANGELOG_FILE) as f:
    
    rx = re.compile('^(\d[^()]+)\s.*$')

    for line in f:
        release_search = rx.search(line)

        if release_search:
            if latest_release:
                break
            else:
                latest_release = os.getenv(TAGGED_RELEASE_ENV_NAME, release_search.group(1))
        else:
            if latest_release:
                latest_changes.append(line)
    
    print(MSG)
    print('Date: %s' % today.strftime("%B %d, %Y"))
    print('Release: %s\n' % latest_release)
    print("".join(latest_changes))
