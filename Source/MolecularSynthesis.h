
/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             MolecularSynthesis
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      MolecularSynthesis

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:             Component
 mainClass:        molecularSynthesis

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <algorithm>

#define SIGNAL_PERIOD 20

// for convenience
using json = nlohmann::json;

//==============================================================================
class MolecularSynthesis : public AudioAppComponent,
									public juce::Slider::Listener,
                                    private Timer
{
public:
	struct Atom		//@ToDo - Extend this to operate in more dimensions? So have arrays of position, velocity, acceleration;
	{
		uint32_t numConnections = 0;
		Atom* connections[300];
		double mass;
		double force[3];
		double position[3];
		double velocity[3];
		double acceleration[3];

		float posX;
		float posY;
	};

	void defaultMolecule(Atom& aMolecule)
	{
		aMolecule.mass = 2.0;
		aMolecule.force[0] = 0.0;
		aMolecule.force[1] = 0.0;
		aMolecule.force[2] = 0.0;
		aMolecule.position[0] = 0.01;
		aMolecule.position[1] = 0.01;
		aMolecule.position[2] = 0.01;
		aMolecule.velocity[0] = 0.0;
		aMolecule.velocity[1] = 0.0;
		aMolecule.velocity[2] = 0.0;
		aMolecule.acceleration[0] = 0.0;
		aMolecule.acceleration[1] = 0.0;
		aMolecule.acceleration[2] = 0.0;
	}

	// Parse .pdb file containing CONECT entries. Populates aMolecules with connections;
	void parsePDB(std::string aPath, Atom aMolecule[])
	{
		std::ifstream flPdb(aPath);

		//size_t numAtoms = 0;
		std::string line;

		while (std::getline(flPdb, line))
		{
			std::string throwaway;
			std::stringstream ss(line);
			ss >> throwaway;
			if (throwaway.find("CONECT") != std::string::npos)
			{
				size_t numBonds = 0;
				uint32_t idxAtom;
				ss >> idxAtom;
				idxAtom -= 1;

				// Default initalization of molecule;
				defaultMolecule(aMolecule[idxAtom]);

				uint32_t idxBond;
				while (ss >> idxBond)
				{
					aMolecule[idxAtom].connections[numBonds++] = &(aMolecule[idxBond-1]);
				}
				aMolecule[idxAtom].numConnections = numBonds;
				numAtoms++;
			}
		}
	}

	// Parse .json file containing custom format for molecule contents and connections. Populates aMolecules with connections;
	void parseJSON(std::string aPath, Atom aMolecule[])
	{
		std::ifstream i(aPath);
		json jsonInput;
		i >> jsonInput;

		numAtoms = jsonInput["molecule"].size();

		for (uint32_t i = 0; i != numAtoms; ++i)
		{
			aMolecule[i].numConnections = jsonInput["molecule"][i]["connections"].size();
			for (uint32_t j = 0; j != aMolecule[i].numConnections; ++j)
			{
				aMolecule[i].connections[j] = &(aMolecule[jsonInput["molecule"][i]["connections"][j]]);
			}
			aMolecule[i].mass = jsonInput["molecule"][i]["mass"];
			aMolecule[i].force[0] = 0.0;
			aMolecule[i].force[1] = 0.0;
			aMolecule[i].force[2] = 0.0;
			aMolecule[i].position[0] = 0.01;
			aMolecule[i].position[1] = 0.01;
			aMolecule[i].position[2] = 0.01;
			aMolecule[i].velocity[0] = 0.0;
			aMolecule[i].velocity[1] = 0.0;
			aMolecule[i].velocity[2] = 0.0;
			aMolecule[i].acceleration[0] = 0.0;
			aMolecule[i].acceleration[1] = 0.0;
			aMolecule[i].acceleration[2] = 0.0;
		}

		aMolecule[inputPos].force[0] = 0.2;
		aMolecule[inputPos].force[1] = 0.2;
		aMolecule[inputPos].force[2] = 0.2;
		aMolecule[inputPos].position[0] = 0.2;
		aMolecule[inputPos].position[1] = 0.2;
		aMolecule[inputPos].position[2] = 0.2;
	}
    //==============================================================================
	MolecularSynthesis()
       #ifdef JUCE_DEMO_RUNNER
        : AudioAppComponent (getSharedAudioDeviceManager (0, 2))
       #endif
    {
        setSize (600, 600);

        for (auto i = 0; i < numElementsInArray (waveValues); ++i)
            zeromem (waveValues[i], sizeof (waveValues[i]));

        // specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
        startTimerHz (60);

		//parseMolecule("../../Source/input.json", molecule);
		//parsePDB("../../Source/resources/graphene_with_bonds.pdb", molecule);

		flOutput.open("data2.bin", std::ios::out | std::ios::binary);

		deltaT = 1 / sampleRate;
		deltaX = 0.00001;
		inputPos = 14;
		outputPos = 34;
		for (uint32_t i = 0; i != (int)sampleRate; ++i)
		{
			input[i] = 0.0;
			if (i < 5)
				input[i] = 50.0;
		}

		// Generate Sawtooth Signal;
		int m = SIGNAL_PERIOD;
		int x = 0;
		int i = 0;
		while (i < m * 3)
		{
			sawtooth[i] = x / (float)m;
			x++;
			x = x % m;
			i++;
		}

		//Init some positions;

		molecule[0].posX = 330;
		molecule[0].posY = 300;
		molecule[1].posX = 270;
		molecule[1].posY = 300;
		molecule[2].posX = 300;
		molecule[2].posY = 270;
		molecule[3].posX = 300;
		molecule[3].posY = 240;
		molecule[4].posX = 270;
		molecule[4].posY = 210;
		molecule[5].posX = 330;
		molecule[5].posY = 210;

		// Radio Buttons;

		//addAndMakeVisible(btnExcite);
		//btnExcite.setBounds(20, 40, getWidth() - 30, 20);
		//btnExcite.onClick = [this] { updateToggleState(&btnExcite, "Excite");   };

		//addAndMakeVisible(btnCreate);
		//btnCreate.setBounds(20, 60, getWidth() - 30, 20);
		//btnCreate.onClick = [this] { updateToggleState(&btnCreate, "Create");   };

		//addAndMakeVisible(btnConnect);
		//btnConnect.setBounds(20, 80, getWidth() - 30, 20);
		//btnConnect.onClick = [this] { updateToggleState(&btnConnect, "Connect");   };

		//addAndMakeVisible(btnSetInputPos);
		//btnSetInputPos.setBounds(20, 100, getWidth() - 30, 20);
		//btnSetInputPos.onClick = [this] { updateToggleState(&btnSetInputPos, "InputPos");   };

		//addAndMakeVisible(btnSetOutputPos);
		//btnSetOutputPos.setBounds(20, 120, getWidth() - 30, 20);
		//btnSetOutputPos.onClick = [this] { updateToggleState(&btnSetOutputPos, "OutputPos");   };

		//btnExcite.setRadioGroupId(idRadioButton);
		//btnCreate.setRadioGroupId(idRadioButton);
		//btnConnect.setRadioGroupId(idRadioButton);
		//btnSetInputPos.setRadioGroupId(idRadioButton);
		//btnSetOutputPos.setRadioGroupId(idRadioButton);

		addAndMakeVisible(btnImpulse);
		btnImpulse.setBounds(20, 40, getWidth() - 30, 20);
		btnImpulse.onClick = [this] { updateToggleState(&btnImpulse, "Impulse");   };

		addAndMakeVisible(btnSin);
		btnSin.setBounds(20, 60, getWidth() - 30, 20);
		btnSin.onClick = [this] { updateToggleState(&btnSin, "Sin");   };

		addAndMakeVisible(btnSaw);
		btnSaw.setBounds(20, 80, getWidth() - 30, 20);
		btnSaw.onClick = [this] { updateToggleState(&btnSaw, "Saw");   };

		btnImpulse.setRadioGroupId(idRadioButton);
		btnSin.setRadioGroupId(idRadioButton);
		btnSaw.setRadioGroupId(idRadioButton);


		// Sliders;

		addAndMakeVisible(lblInputPos);
		lblInputPos.setText("Input Pos: ", juce::dontSendNotification);
		lblInputPos.attachToComponent(&sldInputPos, true);
		
		addAndMakeVisible(sldInputPos);
		sldInputPos.setBounds(20, 140, getWidth() - 30, 20);
		sldInputPos.setRange(0, numAtoms, 1);
		sldInputPos.addListener(this);

		addAndMakeVisible(lblOutputPos);
		lblOutputPos.setText("Output Pos: ", juce::dontSendNotification);
		lblOutputPos.attachToComponent(&sldOutputPos, true);

		addAndMakeVisible(sldOutputPos);
		sldOutputPos.setBounds(20, 160, getWidth() - 30, 20);
		sldOutputPos.setRange(0, numAtoms, 1);
		sldOutputPos.addListener(this);

		addAndMakeVisible(lblWaveSpeed);
		lblWaveSpeed.setText("Wave Speed: ", juce::dontSendNotification);
		lblWaveSpeed.attachToComponent(&sldWaveSpeed, true);

		addAndMakeVisible(sldWaveSpeed);
		sldWaveSpeed.setBounds(20, 180, getWidth() - 30, 20);
		sldWaveSpeed.setRange(0.000001, 1.0);
		sldWaveSpeed.addListener(this);

		addAndMakeVisible(lblGenDamping);
		lblGenDamping.setText("Gen Damping: ", juce::dontSendNotification);
		lblGenDamping.attachToComponent(&sldGenDamping, true);

		addAndMakeVisible(sldGenDamping);
		sldGenDamping.setBounds(20, 200, getWidth() - 30, 20);
		sldGenDamping.setRange(0.0, 2.0);
		sldGenDamping.addListener(this);


    }
	void updateToggleState(juce::Button* button, juce::String name)
	{
		if (name.contains("Excite"))
		{
			auto state = button->getToggleState();
			juce::String stateString = state ? "ON" : "OFF";

			juce::Logger::outputDebugString(name + " Button changed to " + stateString);

			interactiveState = State_Excite;
		}
		else if (name.contains("Create"))
		{
			auto state = button->getToggleState();
			juce::String stateString = state ? "ON" : "OFF";

			juce::Logger::outputDebugString(name + " Button changed to " + stateString);

			interactiveState = State_Create;
		}
		else if (name.contains("Connect"))
		{
			auto state = button->getToggleState();
			juce::String stateString = state ? "ON" : "OFF";

			juce::Logger::outputDebugString(name + " Button changed to " + stateString);

			interactiveState = State_Connect;
		}
		else if (name.contains("InputPos"))
		{
			auto state = button->getToggleState();
			juce::String stateString = state ? "ON" : "OFF";

			juce::Logger::outputDebugString(name + " Button changed to " + stateString);

			interactiveState = State_InputPos;
		}
		else if (name.contains("OutputPos"))
		{
			auto state = button->getToggleState();
			juce::String stateString = state ? "ON" : "OFF";

			juce::Logger::outputDebugString(name + " Button changed to " + stateString);

			interactiveState = State_OutputPos;
		}
		else if (name.contains("Impulse"))
		{
			auto state = button->getToggleState();
			juce::String stateString = state ? "ON" : "OFF";

			juce::Logger::outputDebugString(name + " Button changed to " + stateString);

			exciteState = State_Impulse;
		}
		else if (name.contains("Sin"))
		{
			auto state = button->getToggleState();
			juce::String stateString = state ? "ON" : "OFF";

			juce::Logger::outputDebugString(name + " Button changed to " + stateString);

			exciteState = State_Sin;
		}
		else if (name.contains("Saw"))
		{
			auto state = button->getToggleState();
			juce::String stateString = state ? "ON" : "OFF";

			juce::Logger::outputDebugString(name + " Button changed to " + stateString);

			exciteState = State_Saw;
		}
	}
	void sliderValueChanged(juce::Slider* slider) override
	{
		if (slider == &sldInputPos)
		{
			inputPos = sldInputPos.getValue();
			//sldWaveSpeed.setValue(1.0 / sldWaveSpeed.getValue(), juce::dontSendNotification);
		}
		if (slider == &sldOutputPos)
		{
			inputPos = sldOutputPos.getValue();
			//sldWaveSpeed.setValue(1.0 / sldWaveSpeed.getValue(), juce::dontSendNotification);
		}
		if (slider == &sldWaveSpeed)
		{
			waveSpeed = sldWaveSpeed.getValue();
			waveSpeed = waveSpeed * waveSpeed;
		}
		if (slider == &sldGenDamping)
		{
			genDamp = sldGenDamping.getValue();
			genDamp = genDamp * genDamp;
		}
	}


    ~MolecularSynthesis() override
    {
        shutdownAudio();
    }

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double newSampleRate) override
    {
        sampleRate = newSampleRate;
        expectedSamplesPerBlock = samplesPerBlockExpected;

		parsePDB("../../Source/resources/graphene_with_bonds.pdb", molecule);
		//parsePDB("../../Source/resources/1gwd.pdb", molecule);
		//parsePDB("../../Source/resources/buckyball.pdb", molecule);
		//parsePDB("../../Source/resources/nanotube.pdb", molecule);
		//parsePDB("../../Source/resources/helicene.pdb", molecule);

		isReady = true;
    }

    /*  This method generates the actual audio samples.
        In this example the buffer is filled with a sine wave whose frequency and
        amplitude are controlled by the mouse position.
     */
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();

		auto ind = waveTableIndex;

		auto* channelDataOne = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
		auto* channelDataTwo = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

		if (isReady)
		{
			for (auto n = 0; n < bufferToFill.numSamples; ++n)
			{
				// Prepare input signal;
				if (isExcite)
				{
					int m = SIGNAL_PERIOD;

					float signal = 0.0;
					if (idxSignal < m)
					{
						if (exciteState == State_Sin)
						{
							signal = sin(idxSignal / (float)(m));
							input[n] = signal;
						}
						else if (exciteState == State_Saw)
						{
							signal = sawtooth[idxSignal];
							input[n] = signal;
						}
						idxSignal++;
					}
					else
						idxSignal = 0;
				}
				else
				{
					input[n] = 0.0;
				}

				for (uint32_t i = 1; i != numAtoms; ++i)
				{
					// Old ODE way;
					//if (i == outputPos)
					//	springForceY += input[n];
					//springForceY = -kOde * molecule[i].position[idxRotationN];
					//forceY = springForceY + molecule[i].mass * GRAVITY;

					// Novel way; (This is the way)
					float forceY = 0.0;
					float tempForce = 0.0;
					for (uint32_t j = 0; j != molecule[i].numConnections; ++j)
					{
						tempForce += molecule[i].connections[j]->force[idxRotationN];
					}
					forceY = waveSpeed*waveSpeed * ((tempForce - molecule[i].numConnections * molecule[i].force[idxRotationN]) / (deltaX * deltaX));
					forceY = forceY - (2 * genDamp * ((molecule[i].force[idxRotationN] - molecule[i].force[idxRotationNMOne]) / deltaT));

					//forceY = molecule[i].mass * forceY;
					forceY = forceY * (deltaT*deltaT);
					forceY = forceY + 2 * molecule[i].force[idxRotationN] - molecule[i].force[idxRotationNMOne];
					//forceY *= (1 - damping);
					if (i == inputPos)
						forceY = input[n];

					// @ToDo - Need range check?
					//forceY = std::max(-1.0f, std::min(1.0f, forceY));

					molecule[i].force[idxRotationNPOne] = forceY;
					//molecule[i].velocity[idxRotationNPOne] = velocityY;
					//molecule[i].position[idxRotationNPOne] = molecule[i].position[idxRotationN] + molecule[i].velocity[idxRotationNPOne] * deltaT;		//@Highlight - Used velocity from next step just calculated here.
					//molecule[i].acceleration[idxRotationNPOne] = accelerationY;																			//@ToDo - Currently don't need this???

					if (i == outputPos)
					{
						output[n] = molecule[i].force[idxRotationNPOne];
						//output[n] = molecule[i].position[idxRotationNPOne];
					}
				}
				idxRotationNMOne = (idxRotationNMOne + 1) % 3;
				idxRotationN = (idxRotationN + 1) % 3;
				idxRotationNPOne = (idxRotationNPOne + 1) % 3;

				float sample = output[n];
				channelDataOne[n] = sample;
				channelDataTwo[n] = sample;
				//flOutput.write(&((char)sample), sizeof(float));
			}
		}

        waveTableIndex = (int) (waveTableIndex + bufferToFill.numSamples) % wavetableSize;
    }

    void releaseResources() override
    {
        // This gets automatically called when audio device parameters change
        // or device is restarted.
        stopTimer();
    }


    //==============================================================================
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

        auto nextPos = pos + delta;

        if (nextPos.x < 10 || nextPos.x + 10 > (float) getWidth())
        {
            delta.x = -delta.x;
            nextPos.x = pos.x + delta.x;
        }

        if (nextPos.y < 50 || nextPos.y + 10 > (float) getHeight())
        {
            delta.y = -delta.y;
            nextPos.y = pos.y + delta.y;
        }

        if (! dragging)
        {
            writeInterpolatedValue (pos, nextPos);
            pos = nextPos;
        }
        else
        {
            pos = lastMousePosition;
        }

        // draw a circle
        g.setColour (getLookAndFeel().findColour (Slider::thumbColourId));
		//for (uint32_t i = 0; i != numAtoms; ++i)
		//{
		//	if(i == inputPos)
		//		g.setColour(Colour(0, 255, 0));
		//	else if(i == outputPos)
		//		g.setColour(Colour(255, 0, 0));
		//	else
		//		g.setColour(getLookAndFeel().findColour(Slider::thumbColourId));

		//	//g.fillEllipse(pos.x + molecule[i].posX, pos.y + molecule[i].posY + (molecule[i].force[0] * 70), 20.f, 20.f);
		//	g.fillEllipse(molecule[i].posX, molecule[i].posY + (molecule[i].force[0] * 70), 20.f, 20.f);
		//}

		//for (uint32_t i = 0; i != numLines; ++i)
		//{
		//	g.drawLine(lines[i].pos1[0], lines[i].pos2[0], lines[i].pos1[1], lines[i].pos2[1], 2);
		//}

        /*g.fillEllipse (pos.x - (30), pos.y + (molecule[0].force[0]*70), 20.f, 20.f);
		g.fillEllipse(pos.x + (30), pos.y + (molecule[1].force[0] * 70), 20.f, 20.f);
		g.fillEllipse(pos.x, pos.y - (30) + (molecule[2].force[0] * 70), 20.f, 20.f);
		g.fillEllipse(pos.x, pos.y - (60) + (molecule[3].force[0] * 70), 20.f, 20.f);

		g.fillEllipse(pos.x - (30), pos.y - (90) + (molecule[4].force[0] * 70), 20.f, 20.f);
		g.fillEllipse(pos.x + (30), pos.y - (90) + (molecule[5].force[0] * 70), 20.f, 20.f);*/

		//g.fillEllipse(pos.x - (30), pos.y + (molecule[0].force[0] * 70), 20.f*molecule[0].mass, 20.f*molecule[0].mass);
		//g.fillEllipse(pos.x + (30), pos.y + (molecule[1].force[0] * 70), 20.f*molecule[1].mass, 20.f*molecule[1].mass);
		//g.fillEllipse(pos.x, pos.y - (30) + (molecule[2].force[0] * 70), 20.f*molecule[2].mass, 20.f*molecule[2].mass);
		//g.fillEllipse(pos.x, pos.y - (60) + (molecule[3].force[0] * 70), 20.f*molecule[3].mass, 20.f*molecule[3].mass);

		//g.fillEllipse(pos.x - (30), pos.y - (90) + (molecule[4].force[0] * 70), 20.f*molecule[4].mass, 20.f*molecule[4].mass);
		//g.fillEllipse(pos.x + (30), pos.y - (90) + (molecule[5].force[0] * 70), 20.f*molecule[5].mass, 20.f*molecule[5].mass);

		//g.drawLine(10+pos.x - (30), 10+pos.y + (molecule[1].force[0] * 70), 10+pos.x + (30), 10+pos.y + (molecule[2].force[0] * 70), 2);

		//float centreX = pos.x;
		//float centreY = pos.y;
		//for (uint32_t i = 0; i != numAtoms; ++i)
		//{
		//	float atomX = centreX;
		//	float atomY = centreY;
		//	atomX += 30 * (i % 3);
		//	if((i / 3)
		//	atomY += 30 * (i / 3);
		//	g.fillEllipse(atomX, atomY, 20.f, 20.f);
		//}
    }

    // Mouse handling..
    void mouseDown (const MouseEvent& e) override
    {
		if (interactiveState == State_Excite)
		{
			lastMousePosition = e.position;
			mouseDrag(e);
			dragging = true;

			isExcite = true;

			if (exciteState == State_Impulse)
				input[0] = 1.0;
		}
		else if (interactiveState == State_Create)
		{
			molecule[numAtoms].numConnections = 0;
			defaultMolecule(molecule[numAtoms]);

			molecule[numAtoms].posX = e.position.x;
			molecule[numAtoms].posY = e.position.y;

			++numAtoms;
		}
		else if (interactiveState == State_Connect)
		{
			Atom* firstClosest = &(molecule[0]);
			Atom* secondClosest = &(molecule[1]);

			for (uint32_t i = 0; i != numAtoms; ++i)
			{
				float distMolecule = sqrt((molecule[i].posX - e.position.x) * (molecule[i].posX - e.position.x) + (molecule[i].posY - e.position.y) * (molecule[i].posY - e.position.y));
				float distFirstClosest = sqrt((firstClosest->posX - e.position.x) * (firstClosest->posX - e.position.x) + (firstClosest->posY - e.position.y) * (firstClosest->posY - e.position.y));
				float distSecondClosest = sqrt((secondClosest->posX - e.position.x) * (secondClosest->posX - e.position.x) + (secondClosest->posY - e.position.y) * (secondClosest->posY - e.position.y));
				if (distMolecule < distFirstClosest)
				{
					firstClosest = &(molecule[i]);
				}
				else if (distMolecule < distSecondClosest)
				{
					secondClosest = &(molecule[i]);
				}
			}

			firstClosest->connections[firstClosest->numConnections++] = secondClosest;
			secondClosest->connections[secondClosest->numConnections++] = firstClosest;

			Line line;
			line.pos1[0] = firstClosest->posX+10.0;
			line.pos2[0] = firstClosest->posY + 20.0;
			line.pos1[1] = secondClosest->posX + 10.0;
			line.pos2[1] = secondClosest->posY + 20.0;
			lines[numLines++] = line;
		}
		else if (interactiveState == State_InputPos)
		{
			uint32_t idxInputPos;
			Atom* firstClosest = &(molecule[0]);
			for (uint32_t i = 0; i != numAtoms; ++i)
			{
				float distMolecule = sqrt((molecule[i].posX - e.position.x) * (molecule[i].posX - e.position.x) + (molecule[i].posY - e.position.y) * (molecule[i].posY - e.position.y));
				float distFirstClosest = sqrt((firstClosest->posX - e.position.x) * (firstClosest->posX - e.position.x) + (firstClosest->posY - e.position.y) * (firstClosest->posY - e.position.y));
				if (distMolecule < distFirstClosest)
				{
					idxInputPos = i;
					firstClosest = &(molecule[i]);
				}
			}

			inputPos = idxInputPos;
		}
		else if (interactiveState == State_OutputPos)
		{
			uint32_t idxOutputPos;
			Atom* firstClosest = &(molecule[0]);
			for (uint32_t i = 0; i != numAtoms; ++i)
			{
				float distMolecule = sqrt((molecule[i].posX - e.position.x) * (molecule[i].posX - e.position.x) + (molecule[i].posY - e.position.y) * (molecule[i].posY - e.position.y));
				float distFirstClosest = sqrt((firstClosest->posX - e.position.x) * (firstClosest->posX - e.position.x) + (firstClosest->posY - e.position.y) * (firstClosest->posY - e.position.y));
				if (distMolecule < distFirstClosest)
				{
					idxOutputPos = i;
					firstClosest = &(molecule[i]);
				}
			}

			outputPos = idxOutputPos;
		}
    }

    void mouseDrag (const MouseEvent& e) override
    {
        dragging = true;

		if (e.position != lastMousePosition)
		{
			// calculate movement vector
			delta = e.position - lastMousePosition;
			lastMousePosition = e.position;
		}
			
    }

    void mouseUp (const MouseEvent&) override
    {
        dragging = false;
		isExcite = false;
    }

    void writeInterpolatedValue (Point<float> lastPosition,
                                 Point<float> currentPosition)
    {
        Point<float> start, finish;

        if (lastPosition.getX() > currentPosition.getX())
        {
            finish = lastPosition;
            start  = currentPosition;
        }
        else
        {
            start  = lastPosition;
            finish = currentPosition;
        }

        for (auto i = 0; i < steps; ++i)
        {
            auto p = start + ((finish - start) * i) / (int) steps;

            auto index = (bufferIndex + i) % wavetableSize;
            waveValues[1][index] = yToAmplitude (p.y);
            waveValues[0][index] = xToAmplitude (p.x);
        }

        bufferIndex = (bufferIndex + steps) % wavetableSize;
    }

    float indexToX (int indexValue) const noexcept
    {
        return (float) indexValue;
    }

    float amplitudeToY (float amp) const noexcept
    {
        return (float) getHeight() - (amp + 1.0f) * (float) getHeight() / 2.0f;
    }

    float xToAmplitude (float x) const noexcept
    {
        return jlimit (-1.0f, 1.0f, 2.0f * ((float) getWidth() - x) / (float) getWidth() - 1.0f);
    }

    float yToAmplitude (float y) const noexcept
    {
        return jlimit (-1.0f, 1.0f, 2.0f * ((float) getHeight() - y) / (float) getHeight() - 1.0f);
    }

    void timerCallback() override
    {
        repaint();
    }

private:
    //==============================================================================
    enum
    {
        wavetableSize = 36000,
        steps = 10
    };

    Point<float> pos   = { 299.0f, 299.0f };
    Point<float> delta = { 0.0f, 0.0f };
    int waveTableIndex = 0;
    int bufferIndex    = 0;
    double sampleRate  = 0.0;
    int expectedSamplesPerBlock = 0;
    Point<float> lastMousePosition;
    float waveValues[2][wavetableSize];
    bool dragging = false;

	bool isReady = false;
	bool isExcite = false;

	// Sawtooth;
	uint32_t idxSignal = 0;
	float sawtooth[SIGNAL_PERIOD*3];

	// Lump-Mass-Spring Vars;
	double deltaT = 1 / sampleRate;
	double deltaX = 0.00001;
	int inputPos = 0;
	int outputPos = 0;
	float input[48000];
	float output[48000];

	const double GRAVITY = 10.000;
	double kOde = 704000.0;
	double waveSpeed = 0.015;
	double genDamp = 0.0001;
	double damping = 0.0001;

	int idxRotationNMOne = 0;
	int idxRotationN = 1;
	int idxRotationNPOne = 2;
	uint32_t numAtoms = 0;
	Atom molecule[10000];

	enum Interactive_State
	{
		State_Excite,
		State_Create,
		State_Connect,
		State_InputPos,
		State_OutputPos
	};

	enum Excite_State
	{
		State_Impulse,
		State_Sin,
		State_Saw
	};

	struct Line
	{
		float pos1[2];
		float pos2[2];
	};

	// Interactions;
	Excite_State exciteState = State_Impulse;
	Interactive_State interactiveState = State_Excite;
	int idRadioButton = 1100;
	//juce::ToggleButton btnExcite{ "Excite" };
	//juce::ToggleButton btnCreate{ "Create" };
	//juce::ToggleButton btnConnect{ "Connect" };
	//juce::ToggleButton btnSetInputPos{ "InputPos" };
	//juce::ToggleButton btnSetOutputPos{ "OutputPos" };

	juce::ToggleButton btnImpulse{ "Impulse" };
	juce::ToggleButton btnSin{ "Sin" };
	juce::ToggleButton btnSaw{ "Saw" };

	juce::Label  lblInputPos;
	juce::Slider sldInputPos;

	juce::Label  lblOutputPos;
	juce::Slider sldOutputPos;

	juce::Label  lblWaveSpeed;
	juce::Slider sldWaveSpeed;

	juce::Label  lblGenDamping;
	juce::Slider sldGenDamping;

	uint32_t numLines = 0;
	Line lines[1000];

	std::ofstream flOutput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MolecularSynthesis)
};
