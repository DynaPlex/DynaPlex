import os
import sys
import re
import pytest

parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_directory)
# noinspection PyUnresolvedReferences
from dp.loader import DynaPlex as dp
sys.path.remove(parent_directory)

def test_non_convertible():
    with pytest.raises(TypeError) as exc_info:
        dp.get_mdp(123)

def test_model_factory_missing():
    # Create VarGroup using a Python dictionary. Note typo
    vars = {"id": "LosSales", "p": 9.0, "h": 1.0, "leadtime": 3}
    #Expecting an error when calling get_mdp
    with pytest.raises(RuntimeError, match=re.escape("DynaPlex: No MDP available with identifier \"LosSales\". Use ListMDPs() / list_mdps() to obtain available MDPs.")) as exc_info:
        model = dp.get_mdp(**vars)

def test_model_factory_tests():
    # Create VarGroup using a Python dictionary
    vars = {"id": "lost_sales", "p": 9.0, "h": 1.0, "leadtime": 3, "demand_dist":{"type":"poisson","mean":3.0} }
    try:
        model = dp.get_mdp(**vars)
    except Exception as e:
        pytest.fail(f"Unexpected error: {e}")
    identifier = model.identifier()
    assert identifier.startswith("lost_sales"), f"Expected identifier to start with 'lost_sales', but got '{identifier}'"


def test_filepath():
    subdirs = ['test', 'folder1', 'folder2']
    filename = 'myfile.txt'
    expected_path = os.path.join(dp.io_path(), *subdirs, filename)

    try:
        actual_path = dp.filepath(*subdirs, filename)
    except Exception as e:
        pytest.fail(f"Unexpected error: {e}")

    assert actual_path == expected_path, f"Expected path to be {expected_path}, but got {actual_path}"



def test_model_factory_named_args():
    try:
        model = dp.get_mdp(id="lost_sales",p=9.0,h=1.0,leadtime=3,demand_dist={"type":"poisson","mean":3.0})
    except Exception as e:
        pytest.fail(f"Unexpected error: {e}")   
    identifier = model.identifier()
    assert identifier.startswith("lost_sales"), f"Expected identifier to start with 'lost_sales', but got '{identifier}'"

def test_list_mdps():
    mdp_list = dp.list_mdps()
    assert isinstance(mdp_list, dict)
    assert len(mdp_list) > 0
    assert "lost_sales" in mdp_list
    assert mdp_list["lost_sales"] == "Canonical lost sales problem, see e.g. Zipkin (2008) for a formal description. (parameters: p, h, leadtime, demand_dist.)"
