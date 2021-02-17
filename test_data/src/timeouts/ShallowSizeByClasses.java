package timeouts;

import com.intellij.memory.agent.IdeaNativeAgentProxy;
import common.TestTreeNode;
import common.TimeoutTestBase;

public class ShallowSizeByClasses extends TimeoutTestBase {
    @Override
    protected MemoryAgentErrorCode executeOperation(IdeaNativeAgentProxy proxy) {
        TestTreeNode root = TestTreeNode.createTreeFromString("2 1 1 0 0 0 0");
        return getErrorCode(proxy.getShallowSizeByClasses(new Object[] {TestTreeNode.Impl2.class}));
    }

    public static void main(String[] args) {
        TimeoutTestBase test = new ShallowSizeByClasses();
        test.doTest();
    }
}

