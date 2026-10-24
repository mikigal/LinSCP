#!/usr/bin/python3
"""The S3 server of the tests: moto's server (moto_server), whose requests
have the URL as the client sent it. moto 5.1 verifies signatures against
the URL as werkzeug rebuilds it, with percent-encodings in the query decoded,
so requests with encoded query values (delimiter=%2F, prefixes with spaces)
fail to authenticate.
"""

import sys

import flask
from moto.server import main

REBUILT_URL = flask.Request.url


def raw_url(request):
    raw_uri = request.environ.get('RAW_URI') or request.environ.get('REQUEST_URI')
    if raw_uri and raw_uri.startswith('/'):
        return request.host_url.rstrip('/') + raw_uri
    return REBUILT_URL.__get__(request)


flask.Request.url = property(raw_url)

if __name__ == '__main__':
    sys.exit(main())
