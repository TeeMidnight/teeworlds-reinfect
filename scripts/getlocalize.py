import os, json
import re

Translate = {}
SortTranslate = {}

Path = "data/languages/"

def ReadFile(path):
    File = open(path, "r")
    for line in File.readlines():
        line = line.strip()

        string = re.findall("Localize\s*\(\s*\S*\s*,*\s*\"([^\"]+)*\"\s*\)", line)
        if len(string) > 0:
            for i in string:
                TranslateString : str = i
                Translate.setdefault(TranslateString, "")

    File.close()

def ReadDir(dir):
    for i in os.listdir(dir):
        fulldir = os.path.join(dir, i)
        if os.path.isdir(fulldir):
            ReadDir(fulldir)
        elif os.path.isfile(fulldir):
            ReadFile(fulldir)

def ReadLanguageFile(path):
    for key in SortTranslate:
        SortTranslate[key] = ""

    try:
        langfile = open(path, "r")
        data = json.loads(langfile.read())

        for i in data['translated strings']:
            SortTranslate.setdefault(str(i['key']), "")
            SortTranslate[str(i['key'])] = str(i["value"])

        langfile.close()
    except:
        pass
    

def WriteLanguageFile(path):
    langfile = open(path, "w")

    print("{", end="\n", file=langfile)
    print("\"translated strings\": [", end="\n", file=langfile)

    for key in SortTranslate:
        print("\t{", end="\n", file=langfile)
        print(f"\t\t\"key\": \"{key}\",", end="\n", file=langfile)
        print(f"\t\t\"value\": \"{SortTranslate[key]}\"", end="\n", file=langfile)
        if list(SortTranslate).index(key) == len(list(SortTranslate)) - 1:
            print("\t}", end="\n", file=langfile)
        else:
            print("\t},", end="\n", file=langfile)

    print("]", end="\n", file=langfile)
    print("}", end="\n", file=langfile)

    langfile.close()

if __name__ == '__main__':
    ReadDir("src/game/")
    ReadDir("src/engine/server/")
    
    for key in sorted(Translate):
        SortTranslate.setdefault(key, "")

    languagefiles = ['zh-CN', 'zh-TW']

    for filename in languagefiles:
        if os.path.exists(f"data/languages/{filename}.json") == False:
            WriteLanguageFile(f"data/languages/{filename}.json")
        else:
            ReadLanguageFile(f"data/languages/{filename}.json")
            WriteLanguageFile(f"data/languages/{filename}.json")