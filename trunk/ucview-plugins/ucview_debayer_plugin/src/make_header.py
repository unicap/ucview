import sys


f=open(sys.argv[1])
out="""#ifndef __PLUGIN_UI_H__
#define __PLUGIN_UI_H__
static char* plugin_ui =\
"""
for l in f:
    l = l.replace('"',"'")
    l = l.replace('\n' ,'')
    l = '"'+l+'"\\\n'
    out += l
out += '"";\n'
out += '#endif//__PLUGIN_UI_H__'
f.close()
f=open(sys.argv[2],'w')
f.write(out)
f.close()

