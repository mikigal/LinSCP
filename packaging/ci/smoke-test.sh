#!/bin/bash
# Checks a built LinSCP: linscp-cli runs and reports the version of a release
# build, and all libraries of the GUI are found:
#   packaging/ci/smoke-test.sh VERSION [LINSCP_CLI [LINSCP]]
# Without paths, the installed linscp-cli and linscp.
set -euo pipefail

Version=$1
Cli=${2:-$(command -v linscp-cli)}
Gui=${3:-$(command -v linscp)}

# Nothing of the user's: the configuration goes to a new directory
Home=$(mktemp -d)
Output=$(env HOME="$Home" XDG_CONFIG_HOME="$Home/.config" XDG_CACHE_HOME="$Home/.cache" \
  XDG_DATA_HOME="$Home/.local/share" "$Cli" /help < /dev/null)
Banner=${Output%%$'\n'*}
echo "$Banner"
# "Version 6.7.1.1 (" is not version 6.7.1.10
if [[ "$Banner" != *"Version $Version ("* ]]
then
  echo "linscp-cli is not version $Version" >&2
  exit 1
fi
if [[ "$Banner" == *distribute* ]]
then
  echo "linscp-cli is a development build" >&2
  exit 1
fi

Missing=$(ldd "$Gui" | grep "not found" || true)
if [ -n "$Missing" ]
then
  echo "Libraries of $Gui not found:" >&2
  echo "$Missing" >&2
  exit 1
fi
rm -rf "$Home"
