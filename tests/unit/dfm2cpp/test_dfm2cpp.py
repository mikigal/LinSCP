"""Tests of the generator of the texts of the forms (tools/dfm2cpp)."""

import unittest
from pathlib import Path

import dfm2cpp

ROOT = Path(__file__).resolve().parents[3]

FORM = """object TestDialog: TTestDialog
  Left = 10
  Caption = 'Test dialog'
  Font.Name = 'Tahoma'
  object Panel: TPanel
    Anchors = [akLeft, akTop]
    object Label1: TLabel
      Caption =
        'First line'#13#10'it'#39's ' +
        'continued'
      Hint = 'Short|Long hint'
    end
    object Combo: TComboBox
      Items.Strings = (
        'One'
        'Two'#169)
    end
    object ListView: TListView
      Columns = <
        item
          Caption = 'Name'
          Width = 100
        end
        item
          Width = 50
        end
        item
          Caption = 'Size'
        end>
      Data = {
        0102}
    end
  end
  inherited Button: TButton [0]
    Caption = '&OK'#55357#56832
    OnClick = ButtonClick
  end
end
"""


class ParseTest(unittest.TestCase):
    def test_entries(self):
        self.assertEqual(dfm2cpp.parse(FORM), [
            ('Caption', 'Test dialog'),
            ('Panel.Label1.Caption', "First line\r\nit's continued"),
            ('Panel.Label1.Hint', 'Short|Long hint'),
            ('Panel.Combo.Items.Strings.0', 'One'),
            ('Panel.Combo.Items.Strings.1', 'Two©'),
            ('Panel.ListView.item0.Caption', 'Name'),
            ('Panel.ListView.item2.Caption', 'Size'),
            ('Button.Caption', '&OK\U0001f600')])

    def test_errors(self):
        for text in ['object A: TA\n  Caption = \n', 'object A: TA\n  Caption = ?\nend\n', 'end\n']:
            with self.subTest(text=text):
                with self.assertRaises(dfm2cpp.DfmError):
                    dfm2cpp.parse(text)

    def test_source(self):
        source = dfm2cpp.generate([('Test.dfm', FORM)])
        self.assertIn('  { L"Test.dfm", L"Panel.Combo.Items.Strings.1", L"Two\\u00a9" },', source)
        source.encode('ascii')

    def test_project_forms(self):
        # The keys of the translations are the keys of the forms
        forms = {}
        for path in (ROOT / 'source' / 'forms').glob('*.dfm'):
            forms[path.name] = dict(dfm2cpp.parse(path.read_bytes().decode('utf-8-sig')))
        self.assertIn('Caption', forms['Login.dfm'])
        section = None
        missing = []
        for line in (ROOT / 'translations' / 'PL.ini').read_text(encoding='utf-8-sig').splitlines():
            if line.startswith('['):
                section = line[1:-1]
            elif section is not None and section.endswith('.dfm') and '=' in line:
                key = line.split('=', 1)[0]
                if key not in forms.get(section, {}):
                    missing.append((section, key))
        self.assertEqual(missing, [])


if __name__ == '__main__':
    unittest.main()
