import glob, os, sys, re

if len(sys.argv) < 3:
    print('Not enough parameters!')
    sys.exit();

os.chdir(sys.argv[1])

libs = []
for subpath in sys.argv[2:]:
    libs += glob.glob(subpath + '/**/libQt*.a', recursive=True) + \
            glob.glob(subpath + '/**/libq*.a', recursive=True)

for subpath in sys.argv[2:]:
    for filename in glob.glob(subpath + '/*.prl'):
        with open(filename, 'r') as file:
            file_content = file.read()
            new_content = re.sub(r'(?m)^QMAKE_PRL_BUILD_DIR.*\n?', '', file_content)
            for lib in libs:
                wrong_path = lib.split('/')[-1].split('.')[0]
                new_content = re.sub(
                    r'\s(\/' + wrong_path + '\.a)+',
                    ' ' + sys.argv[1] + '/' + lib,
                    new_content,
                )
        with open(filename, 'w') as file:
            file.write(new_content)
