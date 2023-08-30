import os
import sys
import re
import pytest

# Same path modification as your code
parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_directory)
from dp.loader import DynaPlex as dp
sys.path.remove(parent_directory)

def test_non_convertible():
    with pytest.raises(TypeError) as exc_info:
        dp.get_mdp(123)

def test_model_factory_missing():
    # Create VarGroup using a Python dictionary. Note typo
    vars = {"id": "LosSales", "p": 9.0, "h": 1.0, "leadtime": 3}
    # Expecting an error when calling get_mdp
    with pytest.raises(RuntimeError, match=re.escape("DynaPlex: No MDP available with identifier \"LosSales\". Use ListMDPs() / list_mdps() to obtain available MDPs.")) as exc_info:
        model = dp.get_mdp(vars)

def test_model_factory_tests():
    # Create VarGroup using a Python dictionary
    vars = {"id": "lost_sales", "p": 9.0, "h": 1.0, "leadtime": 3, "demand_dist":{"type":"poisson","mean":3.0} }
    try:
        model = dp.get_mdp(vars)
    except Exception as e:
        pytest.fail(f"Unexpected error: {e}")
    identifier = model.identifier()
    assert identifier.startswith("lost_sales"), f"Expected identifier to start with 'lost_sales', but got '{identifier}'"




def test_model_factory_named_args():
    try:
        model = dp.get_mdp(id="lost_sales",p=9.0,h=1.0,leadtime=3,demand_dist={"type":"poisson","mean":3.0})
    except Exception as e:
        pytest.fail(f"Unexpected error: {e}")   
    identifier = model.identifier()
    assert identifier.startswith("lost_sales"), f"Expected identifier to start with 'lost_sales', but got '{identifier}'"

def test_convert_to_string_with_settings():
    settings = {
        "ape": 1,
        "donkey": 0.5,
        "long_setting": -120000231,
        "test": {"test": list(range(100000))},
        "names": ({"name": "bill"}, {"name2": "bill"})
    }

    expected_output = """{
    "ape": 1,
    "donkey": 0.5,
    "long_setting": -120000231,
    "test": {
        "test": [0, 1, ... (99997 omitted) ..., 99999]
    },
    "names": [{
            "name": "bill"
        }, {
            "name2": "bill"
        }]
}"""
    result = dp.test_param(settings)
    assert result == expected_output, f"Expected:\n{expected_output}\n\nGot:\n{result}"


def test_convert_to_string_with_params():
    expected_output = """{
    "test": ["asdf", "asdf", "asf"],
    "ace": "asdf"
}"""
    result = dp.test_param(test=['asdf', 'asdf', 'asf'], ace='asdf')
    assert result == expected_output, f"Expected:\n{expected_output}\n\nGot:\n{result}"


def test_get_var_group():
    expected_output = {'type': 'geom', 'mean': 5}
    result = dp.get_var_group()
    assert result == expected_output, f"Expected {expected_output}, but got {result}"
