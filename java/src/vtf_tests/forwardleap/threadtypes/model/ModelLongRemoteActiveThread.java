package vtf_tests.forwardleap.threadtypes.model;

public class ModelLongRemoteActiveThread extends ModelThread {
	
	public ModelLongRemoteActiveThread(int iterations) {
		super(iterations);
	}
		
    @virtualtime.ModelPerformance(
    		jmtModelFilename="../src/models/vtest/long_remote_service.jsimg",
    		replaceMethodBody=false)  	
	protected void execute() {
    	updateTemp(1);
	}	
    
    public boolean requiresResource() {
    	return false;
    }
    
    protected int resourceUsageInMsPerIteration() {
    	return 220;
    }
}
