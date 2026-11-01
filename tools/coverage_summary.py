"""Summarise GCC gcov JSON. Run after gcov -j on each application .gcno file."""
import argparse,gzip,json
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('input');parser.add_argument('output');args=parser.parse_args()
rows=[]
for p in Path(args.input).glob('*.gcov.json.gz'):
 with gzip.open(p,'rt') as f:data=json.load(f)
 for file in data['files']:
  name=Path(file['file']).name
  if name not in {'AssessmentService.cpp','User.cpp','DeadlineRepository.cpp','Database.cpp','Security.cpp'}:continue
  lines=file['lines'];covered=sum(x['count']>0 for x in lines);total=len(lines)
  rows.append({'file':name,'covered_lines':covered,'executable_lines':total,'percent':round(100*covered/total,2) if total else None})
result={'tool':'GCC gcov JSON','scope':'Application C++ sources linked into lms_core; excludes main, SQLite, Catch2 and standard library. Mixed-layer code is not an isolated business-logic layer.','files':sorted(rows,key=lambda x:x['file'])}
result['covered_lines']=sum(x['covered_lines'] for x in rows);result['executable_lines']=sum(x['executable_lines'] for x in rows)
result['percent']=round(100*result['covered_lines']/result['executable_lines'],2) if result['executable_lines'] else None
Path(args.output).write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
