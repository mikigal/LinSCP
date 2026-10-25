#!/usr/bin/python3
"""Sets up the S3 server of the tests (moto, in memory) after it starts:
an IAM user with an access key that may do everything, then authentication
is enforced. The key is written to the credentials file (ACCESS_KEY SECRET),
which the tests read. Run by the service (ExecStartPost) on the test VM.
"""

import json
import sys
import time
import urllib.request

import boto3

ENDPOINT = 'http://127.0.0.1:%s' % sys.argv[1]
CREDENTIALS = sys.argv[2]
USER = 'linscp-tests'


def wait_for_server():
    for _ in range(300):
        try:
            urllib.request.urlopen(ENDPOINT + '/moto-api/data.json', timeout=5)
            return
        except OSError:
            time.sleep(0.2)
    raise SystemExit('the S3 server does not respond')


def main():
    wait_for_server()
    # (Any credentials, as authentication is not enforced yet)
    iam = boto3.client('iam', endpoint_url=ENDPOINT, region_name='us-east-1',
                       aws_access_key_id='bootstrap', aws_secret_access_key='bootstrap')
    iam.create_user(UserName=USER)
    iam.put_user_policy(UserName=USER, PolicyName='everything', PolicyDocument=json.dumps({
        'Version': '2012-10-17',
        'Statement': [{'Effect': 'Allow', 'Action': '*', 'Resource': '*'}],
    }))
    key = iam.create_access_key(UserName=USER)['AccessKey']
    with open(CREDENTIALS, 'w') as file:
        file.write('%s %s\n' % (key['AccessKeyId'], key['SecretAccessKey']))
    request = urllib.request.Request(
        ENDPOINT + '/moto-api/reset-auth', data=b'0', method='POST', headers={'Content-Type': 'text/plain'})
    urllib.request.urlopen(request, timeout=10)


if __name__ == '__main__':
    main()
