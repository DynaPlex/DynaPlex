#include "mdp.h"

template<MDPModel>
class MDPAdapter : mdp
{
	
public:
	static MDPAdapter GetAdapter(MDPModel model)
	{
		return MDPAdapter{ model };
	}
	
	
	override void write()
	{
		MDPModel.dowrite();
	}
private:
	MDPModel model;
	MDPAdapter(MDPModel model)
		model{model}
	{

	}

};