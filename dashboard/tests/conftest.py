import sys
from pathlib import Path

# Allow `import app` / `import figures` when pytest runs from the repository root
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
