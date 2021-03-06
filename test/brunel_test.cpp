//
// Created by Arnaud Dhaene on 16.10.17.
//

#include "../googletest/include/gtest/gtest.h"
#include "../src/Neuron.h"
#include "../src/Network.h"

constexpr float EPSILON = 1E-5;

/*!
 * @brief test verifying that :
 * neuron spikes at correct times
 * refractory period is respected
 * potential restarts after refractory period
 * neuron doesn't spike when current is equal to 1
 */
TEST(NeuronTest, MembranePotential) {

    Network simulation(1, true, true, true, false, false); //! we will test with 1 neuron for the membrane test

    /// case that current is 1 picoAmper

    simulation.setCurrent(1, 0, 0, 500);

    std::vector<Neuron*> neurons(simulation.run(500));

    std::vector<double> potentials(neurons[0]->getPotentials());

    EXPECT_LT(neurons[0]->getSpikes().size(), EPSILON);
    EXPECT_GT(fabs(potentials[2580] - 20), EPSILON);
    EXPECT_LT(fabs(potentials[4999] - 20), EPSILON);


    /// case that current is 1.1 picoAmper
    Network simulation1(1, true, true, true, false, false);

    simulation1.setCurrent(1.1, 0, 0, 500);

    std::vector<Neuron*> neurons1(simulation1.run(500));

    std::vector<double> potentials1(neurons1[0]->getPotentials());

    /// Spike number
    EXPECT_NEAR(neurons1[0]->getSpikes().size(), 10, EPSILON);

    /// Spike 1
    EXPECT_NEAR(potentials1[479], 20, 1e-2);
    for(unsigned int i(480); i < (480 + C::REFRACTORY_TIME); ++i) {
        EXPECT_LT(fabs(potentials1[i]), EPSILON);
    }
    EXPECT_GT(fabs(potentials1[480 + C::REFRACTORY_TIME]), EPSILON);

    /// Spike 2
    EXPECT_NEAR(potentials1[979], 20, 1e-2);
    for(unsigned int i(980); i < (98 + C::REFRACTORY_TIME); ++i) {
        EXPECT_LT(fabs(potentials1[i]), EPSILON);
    }
    EXPECT_GT(fabs(potentials1[980 + C::REFRACTORY_TIME]), EPSILON);

}

/*!
 * @brief test verifying that :
 * current respects start and stop times
 * sends correct unvarying current value
 */
TEST(NeuronTest, Current) {

    /// Current instance
    Network simulation(1, true, true, false, false, false);

    simulation.setCurrent(1.5, 0, 0, 400);

    std::vector<Neuron*> neurons(simulation.run(1000));


    /// Testing value variation in Simulation to check if simulation does not affect current
    for(unsigned int i(0); i < 4000; ++i) {
        EXPECT_LT(fabs(simulation.getCurrent(0, i) - 1.5), EPSILON);
    }
    for(unsigned int j(4001); j < 10000; ++j) {
        EXPECT_LT(fabs(simulation.getCurrent(0, j)), EPSILON);
    }

}

/*!
 * @brief test verifying that :
 * simulation does not affect buffer size
 * buffer erases value after transmitting spike
 */
TEST(NeuronTest, Buffer) {

    /// Instance
    Network simulation(2, true, true, false, false, false);

    simulation.setCurrent(1.1, 0, 0, 400);
    simulation.setCurrent(0, 1, 0, 400);

    /// we start simulation until first spike and verify that buffer size does not change
    for(unsigned int i(0); i < 481; ++i) {
        simulation.loop();
    }
    /// Spike 1 occurs at time 480 - neuron resets at time 481

    /// we verify that neuron writes into buffer of other neuron
    EXPECT_LT(simulation.getNeuron(1)->b_amplitude(480 + C::DELAY) - C::J_AMP_EXCITATORY, EPSILON);

    /// we then continue simulation - without exceding buffer size
    for(unsigned int i(480); i < 480 + C::DELAY; ++i) {
        simulation.loop();
    }

    /// verifying that buffer erases transmitted spike
    EXPECT_LT(simulation.getNeuron(1)->b_amplitude(480 + C::DELAY), EPSILON);

}

/*!
 * @brief test verifying that:
 * neuron 0 transmits spike to neuron 1 with appropriate delay
 * neuron 1, having inCurrent of 1, spikes at reception of spike
 * neuron 1 transmits spik to neuron 2
 */
TEST(NetworkTest, ConnectionTransmittance) {

    /// Instance
    Network simulation(3, true, true, true, false, false);

    simulation.setCurrent(1.1, 0, 300, 400);
    simulation.setCurrent(1, 1, 0, 500);
    simulation.setCurrent(0, 2, 0, 500);

    simulation.getNeuron(0)->addConnection(1);
    simulation.getNeuron(0)->addConnection(2);

    std::vector<Neuron*> neurons(simulation.run(500));

    std::vector<double> potentials0(neurons[0]->getPotentials());

    std::vector<double> potentials1(neurons[1]->getPotentials());

    std::vector<double> potentials2(neurons[2]->getPotentials());

	/// Spike 1
    EXPECT_NEAR(potentials0[3479], 20, 1e-2);
    EXPECT_LT(fabs(potentials0[3480]), EPSILON);
    EXPECT_GT(fabs(potentials0[3480 + C::REFRACTORY_TIME + 1]), EPSILON);

	/// Spike 1 Transmission to neuron 1 - will spike to 1st transmission
    EXPECT_NEAR(potentials1[3479 + C::DELAY], 20, 1e-2);
    EXPECT_GT(fabs(potentials1[3480 + C::DELAY + C::REFRACTORY_TIME + 1]), EPSILON);
    
    /// Spike Transmission to neuron 2 - small spike (0.1 mV)
    EXPECT_NEAR(potentials2[3479 + C::DELAY + 1], 0.1, EPSILON);

}

/*!
 * @brief test verifying that:
 * network with connections has approximately half
 * the amount of spikes per neuron that network
 * without connections
 */
TEST(NetworkTest, InhibitionTest)  {

    /// instanciation
    Network simulation1(12500, false, false, true, true, false);
    Network simulation2(12500, false, false, true, true, true); //! with connections!

    std::vector<Neuron*> results1(simulation1.run(100));

    double mean1(0);

    for(auto neuron : results1) {
        mean1 += neuron->getSpikes().size();
    }

    mean1 /= results1.size();

    std::vector<Neuron*> results2(simulation2.run(100));

    double mean2(0);

    for(auto neuron : results2) {
        mean2 += neuron->getSpikes().size();
    }

    mean2 /= results2.size();

    /// expected values are 6-8 and 3-4 for 100 ms of simulation time
    EXPECT_NEAR(mean1, 2 * mean2, 1);
}

/*!
 * @brief test verifying that:
 * average number of outgoing connections = number of incoming connections
 * connections are random and bound
 */
TEST(NetworkTest, ConnectionGeneration) {

    /// instanciation - connection generation is done in Network constructor
    Network simulation(12500, false, false, true, true, true);

    /// we run the simulation during 1 ms in order to get fast access to ptrs on Neurons
    std::vector<Neuron*> results(simulation.run(1));

    /// We check that total number of connections is 12500 * 1250
    /// (TOTAL * CONNECTIONS_PER_NEURON)
    auto total(0);

    for(Neuron* neuron : results) {

        total += neuron->getConnections().size();

        /// checking that connections are correctly bound
        for (unsigned int connection : neuron->getConnections()) {
            EXPECT_LT(connection, 12500);
            EXPECT_GE(connection,     0);
        }

    }

    /*
     * this is equivalent to checking that :
     * the average number of outgoing connections per neuron in the entire network
     * is equal to the number of incoming connections for each neuron
     * as it is a constant, C::C_TOTAL
     */
    EXPECT_EQ(total / C::SIMULATION_SIZE, C::C_TOTAL);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
