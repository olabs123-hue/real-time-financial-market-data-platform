import unittest
from app.models import validate
SAMPLE=dict(symbol='ACME',sequence=1,side='B',price_ticks=100,quantity=2,best_bid_ticks=100,best_ask_ticks=0)
class ModelTests(unittest.TestCase):
 def test_valid(self): self.assertEqual(validate(SAMPLE).sequence,1)
 def test_reject_negative(self):
  with self.assertRaises(ValueError): validate({**SAMPLE,'quantity':-1})
 def test_reject_bool(self):
  with self.assertRaises(ValueError): validate({**SAMPLE,'sequence':True})
 def test_reject_unknown(self):
  with self.assertRaises(ValueError): validate({**SAMPLE,'extra':1})
if __name__=='__main__':unittest.main()
