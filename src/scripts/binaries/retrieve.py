import json
import sys
import shutil
import tempfile
import urllib.request
import re
from pyunpack import Archive
from packaging.version import Version, LegacyVersion
import pprint
pp = pprint.PrettyPrinter(indent=4)
binType = sys.argv[1]
binRegx = {
    'mac': ['x86_64-apple-darwin.*', 'i[3456]86-apple-darwin.*'],
    'win': ['i[3456]86-.*mingw32', 'i[3456]86-.*cygwin'],
    'linux-32': ['i[3456]86-.*linux-gnu'],
    'linux-64': ['x86_64-.*linux-gnu'],
    'linux-arm-32': ['arm.*-linux-gnueabihf'],
    'linux-arm-64': ['aarch64.*-linux-gnu*']
}
with open('package_index.json') as json_file:
    data = json.load(json_file)['packages'][0]['tools']
    data = sorted(data, key=lambda x: LegacyVersion(
        x['name']+x['version']), reverse=True)
    avrdude = None
    for x in data:
        if x['name'] == 'avrdude' and avrdude is None:
            avrdude = x
for s in avrdude['systems']:
    for r in binRegx[binType]:
        if re.match(r, s['host']):
            pp.pprint(s)
            with urllib.request.urlopen(s['url']) as response:
                with tempfile.NamedTemporaryFile(delete=False) as tmp_file:
                    shutil.copyfileobj(response, tmp_file)
                    Archive(tmp_file.name).extractall(binType)