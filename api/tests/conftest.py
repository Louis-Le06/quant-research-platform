import sys
from pathlib import Path

# Allow `import app.main` when pytest runs from the repository root
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
