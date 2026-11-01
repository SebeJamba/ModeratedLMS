"""Run a synthetic end-to-end CR1 workflow in a fresh directory."""
import argparse, subprocess, tempfile
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument('executable');parser.add_argument('transcript')
args=parser.parse_args()
commands='admin\nAdmin123!\n1\nlecturer\nLecturer123!\nLECTURER\n1\n1\nstudent\nStudent123!\nSTUDENT\n1\n4\nCS4433\nSoftware Engineering\n1 2\n5\nCS4433\n3\n3\nlecturer\nLecturer123!\n1\nCS4433\nEssay\n2026-11-01 12:00:00\n100\n4\n1\n2026-11-03 12:00:00\n3\nstudent\nStudent123!\n1\n4\nexit\n'
with tempfile.TemporaryDirectory(prefix='lms-smoke-',dir=Path.cwd()) as directory:
    result=subprocess.run([str(Path(args.executable).resolve())],input=commands,text=True,capture_output=True,cwd=directory,timeout=20)
    Path(args.transcript).write_text(result.stdout+result.stderr)
    assert result.returncode==0, result.stderr
    assert 'Effective deadline: 2026-11-03 12:00:00' in result.stdout
print('PASS: real console workflow displays the class extension')
