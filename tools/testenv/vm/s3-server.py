#!/usr/bin/python3
"""The S3 server of the tests: moto's server (moto_server), with two fixes
of moto 5.1:

- The requests have the URL as the client sent it. moto verifies signatures
  against the URL as werkzeug rebuilds it, with percent-encodings in the
  query decoded, so requests with encoded query values (delimiter=%2F,
  prefixes with spaces) fail to authenticate.
- The ACL of an object can be set by an access control policy in the body
  of the request (moto takes only the canned ACL of the headers, and the
  ACL of the bucket otherwise), as OpenSCP sets it.
"""

import sys

import flask
from moto.s3.responses import S3Response
from moto.server import main

REBUILT_URL = flask.Request.url


def raw_url(request):
    raw_uri = request.environ.get('RAW_URI') or request.environ.get('REQUEST_URI')
    if raw_uri and raw_uri.startswith('/'):
        return request.host_url.rstrip('/') + raw_uri
    return REBUILT_URL.__get__(request)


flask.Request.url = property(raw_url)

HEADERS_PUT_OBJECT_ACL = S3Response.put_object_acl


def put_object_acl(self):
    if (self._acl_from_headers(self.headers) is None) and self.body:
        acl = self._acl_from_body()
        self.backend.put_object_acl(self.bucket_name, self.parse_key_name(), acl)
        return 200, self._get_cors_headers_other(), ''
    return HEADERS_PUT_OBJECT_ACL(self)


S3Response.put_object_acl = put_object_acl

if __name__ == '__main__':
    sys.exit(main())
