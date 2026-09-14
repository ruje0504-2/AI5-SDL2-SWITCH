#!/usr/bin/env python3
"""Run real VM regression fixtures against an existing desktop build.
Use private AI5_ISAKU_SWITCH_BUILD/AI5_ISAKU_SWITCH_TEST c_args for port tests;
scope can also be run against the ordinary default build. No game data is shipped.
"""
import argparse,json,os,pathlib,shlex,subprocess
p=argparse.ArgumentParser();p.add_argument('--build',required=True,type=pathlib.Path)
p.add_argument('--data',type=pathlib.Path);p.add_argument('--chs-font',type=pathlib.Path)
p.add_argument('tests',nargs='+');a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[2];build=a.build.resolve()
if 'runtime_effects' in a.tests and not a.data:p.error('runtime_effects requires --data')
if 'chs_text' in a.tests and not a.chs_font:p.error('chs_text requires --chs-font')
env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SDL_RENDER_DRIVER='software',ISAKU_TEST_FONT=str(root/'fonts/DotGothic16-Regular.ttf'))
if a.data:env['ISAKU_TEST_DATA']=str(a.data.resolve())
if a.chs_font:env['ISAKU_CHS_FONT']=str(a.chs_font.resolve())
os.chdir(build);out=build/'isaku-tests';out.mkdir(exist_ok=True)
entry=next(x for x in json.load(open('compile_commands.json')) if x['file'].endswith('/src/main.c'))
base=shlex.split(entry['command']);clean=[];skip=False
for x in base:
 if skip:skip=False;continue
 if x in ['-MF','-MQ','-MT']:skip=True;continue
 if x in ['-MD','-MMD']:continue
 clean.append(x)
def compile(source,target,main=False):
 cmd=clean.copy();cmd[cmd.index('-o')+1]=str(target);cmd[-1]=str(source)
 if main:cmd.insert(1,'-Dmain=ai5_application_main')
 subprocess.run(cmd,check=True)
compile(pathlib.Path(entry['file']),out/'main.o',True)
link=shlex.split(subprocess.check_output(['ninja','-t','commands','ai5'],text=True).splitlines()[-1])
for name in a.tests:
 source=root/'tests/isaku-switch'/f'{name}.c'
 compile(source,out/f'{name}.o')
 cmd=link.copy();cmd[cmd.index('ai5.p/src_main.c.o')]=str(out/'main.o');cmd.insert(cmd.index('-o'),str(out/f'{name}.o'));cmd[cmd.index('-o')+1]=str(out/name)
 subprocess.run(cmd,check=True);subprocess.run([str(out/name)],env=env,check=True,timeout=30)
