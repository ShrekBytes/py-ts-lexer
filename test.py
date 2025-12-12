# This is a sample Python file
# Testing the lexical analyzer

"""
This is a multiline docstring
for testing purposes
"""

def calculate(x, y):
    # Add two numbers
    result = x + y
    return result

# Variable with type hints
count: int = 3.14  # Type mismatch: int declared, float assigned
name: str = 42     # Type mismatch: str declared, int assigned

# Misspelled keywords
pritn("Hello")     # Should be 'print'
defn something():  # Should be 'def'
    pass

# Undeclared variable usage
total = countr + 5  # 'countr' is undeclared

# Invalid operators
if x =< 10:        # Should be <=
    pass

if y === 5:        # Not valid in Python
    pass

'''
Another multiline
comment here
'''

