# c 2023-12-05
# m 2024-02-25

import subprocess

include_dir = 'dependencies/include'
lib_dir = 'dependencies/lib'
lib = '-lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf'

def main():
    # subprocess.call('taskkill /im tut.exe')
    # subprocess.call(f'g++ -c src/tut.cpp -I {include_dir}')
    # subprocess.call(f'g++ tut.o -o tut -L {lib_dir} {lib}')

    subprocess.call('taskkill /im 416inputs.exe')
    subprocess.call(f'g++ -c src/main.cpp -I {include_dir}')
    subprocess.call(f'g++ main.o -o 416inputs -L {lib_dir} {lib}')

if __name__ == '__main__':
    main()
