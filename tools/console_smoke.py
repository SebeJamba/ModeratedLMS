"""Exercise login, CR1, grading, moderation and release using an isolated database."""
import argparse, subprocess, tempfile, sqlite3
from contextlib import closing
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('executable');parser.add_argument('transcript');args=parser.parse_args()
commands='admin\nAdmin123!\n1\nlecturer\nLecturer123!\nLECTURER\n1\n1\nstudent\nStudent123!\nSTUDENT\n1\n1\nhod\nHod123!\nHOD\n1\n4\nCS4433\nSoftware Engineering\n1 2\n5\nCS4433\n3\n3\nlecturer\nLecturer123!\n1\nCS4433\nEssay\n2099-11-01 12:00:00\n100\n4\n1\n2099-11-03 12:00:00\n3\nstudent\nStudent123!\n2\n1\nessay.txt\n4\nlecturer\nLecturer123!\n6\n2\n1\n80\nDetailed written feedback\nInitial marking\n3\nstudent\nStudent123!\n3\n4\nhod\nHod123!\n2\n1\n75\nReviewed rubric\n5\n1\n4\nstudent\nStudent123!\n1\n3\n4\nexit\n'
with tempfile.TemporaryDirectory(prefix='lms-smoke-',dir=Path(args.transcript).resolve().parent) as directory:
    result=subprocess.run([str(Path(args.executable).resolve())],input=commands,text=True,capture_output=True,cwd=directory,timeout=30)
    Path(args.transcript).write_text(result.stdout+result.stderr)
    assert result.returncode==0, result.stderr
    assert '[ERROR]' not in result.stdout,result.stdout
    assert 'Effective deadline: 2099-11-03 12:00:00' in result.stdout
    release=result.stdout.index('[SUCCESS] Approved mark and feedback released.')
    assert 'Final: 75.000000' not in result.stdout[:release]
    assert 'Detailed written feedback' not in result.stdout[:release]
    assert 'Final: 75.000000' in result.stdout[release:]
    assert 'Detailed written feedback' in result.stdout[release:]
    with closing(sqlite3.connect(Path(directory)/'lms.db')) as db:
        assert db.execute('SELECT COUNT(*) FROM AuditLog WHERE submissionID=1').fetchone()[0]==3
        assert db.execute('SELECT status FROM Submissions WHERE submissionID=1').fetchone()[0]=='RELEASED'
    # Existing users keep their accounts; successful legacy login upgrades its hash.
    salt='000102030405060708090a0b0c0d0e0f'; legacy=5381
    for character in salt+':Admin123!':legacy=((legacy*33)+ord(character)) & 0xffffffff
    with closing(sqlite3.connect(Path(directory)/'lms.db')) as db:
        db.execute('UPDATE Users SET passwordHash=? WHERE username=?',(salt+'$'+format(legacy,'x'),'admin'));db.commit()
    upgraded=subprocess.run([str(Path(args.executable).resolve())],input='admin\nAdmin123!\n3\nexit\n',text=True,capture_output=True,cwd=directory,timeout=30)
    assert upgraded.returncode==0
    assert '[SUCCESS] Welcome back, admin!' in upgraded.stdout
    with closing(sqlite3.connect(Path(directory)/'lms.db')) as db:
        assert db.execute("SELECT passwordHash FROM Users WHERE username='admin'").fetchone()[0].startswith('pbkdf2$600000$')
    with Path(args.transcript).open('a') as transcript:transcript.write('\nLEGACY PASSWORD UPGRADE CHECK\n'+upgraded.stdout+upgraded.stderr)
print('PASS: assessment release gate, audit records and automatic legacy password upgrade')
