#include "MDP_Implementation.h"
#include "utilities.h"
#include "LibraryComponent.h"
int main()
{
	MDP_Implementation mdp_impl(2);
	auto mdp = Dynaplex::Convert(mdp_impl);

	LibraryComponent comp{mdp};
	comp.write();
	return 0;
}
