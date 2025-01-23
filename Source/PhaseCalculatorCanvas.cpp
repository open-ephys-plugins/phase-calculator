/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
Copyright (C) 2018 Translational NeuroEngineering Laboratory, MGH

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "PhaseCalculatorCanvas.h"
#include "PhaseCalculatorEditor.h"

namespace PhaseCalculator
{
    Canvas::Canvas(Node* pc)
        : Visualizer ((GenericProcessor*)pc)
        , processor(pc)
        , viewport(new Viewport())
        , canvas(new Component("canvas"))
        , rosePlotOptions(new Component("rosePlotOptions"))
        , rosePlot(new RosePlot(this))
    {
        refreshRate = 5;

        canvas->addAndMakeVisible(rosePlot.get());

        // populate rosePlotOptions
        const Font textFont = FontOptions("Inter", "Semi Bold", 18.0f);
        const int textHeight = 25;
        const int indent = 10;
        int xPos = indent;
        int yPos = 10;

        cChannelLabel = std::make_unique<Label>("cChannelLabel", "Data channel:");
        cChannelLabel->setBounds(xPos, yPos, 120, textHeight);
        cChannelLabel->setFont(textFont);
        rosePlotOptions->addAndMakeVisible(cChannelLabel.get());

        cChannelBox = std::make_unique<ComboBox>("cChannelBox");
        cChannelBox->setTooltip(cChanTooltip);
        cChannelBox->setBounds(xPos, yPos += textHeight + 2, 80, textHeight);
        cChannelBox->addListener(this);
        rosePlotOptions->addAndMakeVisible(cChannelBox.get());

        eChannelLabel = std::make_unique<Label>("eChannelLabel", "Event Line:");
        eChannelLabel->setBounds(xPos += 140, yPos = indent, 120, textHeight);
        eChannelLabel->setFont(textFont);
        rosePlotOptions->addAndMakeVisible(eChannelLabel.get());


        eChannelBox = std::make_unique<ComboBox>("eChannelBox");
        eChannelBox->setBounds(xPos, yPos += textHeight + 2, 80, textHeight);
        eChannelBox->addItem("None", 1);
        for (int chan = 1; chan <= 8; ++chan)
        {
            eChannelBox->addItem(String(chan), chan + 1);
        }
        eChannelBox->setSelectedId(1, dontSendNotification);
        eChannelBox->addListener(this);
        rosePlotOptions->addAndMakeVisible(eChannelBox.get());

        numBinsLabel = std::make_unique<Label>("numBinsLabel", "Number of bins:");
        numBinsLabel->setBounds(xPos = indent, yPos += 2 * textHeight, 200, textHeight);
        numBinsLabel->setFont(textFont);
        rosePlotOptions->addAndMakeVisible(numBinsLabel.get());

        numBinsSlider = std::make_unique<Slider>();
        numBinsSlider->setSliderStyle(Slider::LinearHorizontal);
        numBinsSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 40, 25);
        numBinsSlider->setBounds(xPos, yPos += textHeight + 2, optionsWidth - 2 * indent, 50);
        numBinsSlider->setRange(1, RosePlot::maxBins, 1);
        numBinsSlider->setValue(RosePlot::startNumBins);
        numBinsSlider->addListener(this);
        rosePlotOptions->addAndMakeVisible(numBinsSlider.get());

        clearButton = std::make_unique<UtilityButton>("Clear Plot");
        clearButton->setFont (FontOptions(16.0f));
        clearButton->addListener(this);
        clearButton->setBounds((optionsWidth - 80) / 2, yPos += 55, 80, 30);
        rosePlotOptions->addAndMakeVisible(clearButton.get());

        referenceLabel = std::make_unique<Label>("referenceLabel", "Phase reference:");
        int refLabelWidth = textFont.getStringWidth(referenceLabel->getText());
        referenceLabel->setBounds(xPos = indent, yPos += 45, refLabelWidth, textHeight);
        referenceLabel->setFont(textFont);
        rosePlotOptions->addAndMakeVisible(referenceLabel.get());

        referenceEditable = std::make_unique<CustomTextBox>("referenceEditable", String(RosePlot::startReference), "0123456789");
        referenceEditable->setEditable(true);
        referenceEditable->addListener(rosePlot.get());
        referenceEditable->setBounds(xPos += refLabelWidth + 5, yPos, 60, textHeight);
        referenceEditable->setTooltip(refTooltip);
        rosePlotOptions->addAndMakeVisible(referenceEditable.get());

        countLabel = std::make_unique<Label>("countLabel");
        countLabel->setBounds(xPos = indent, yPos += 45, optionsWidth, textHeight);
        countLabel->setFont(textFont);

        meanLabel = std::make_unique<Label>("meanLabel");
        meanLabel->setBounds(xPos, yPos += textHeight, optionsWidth, textHeight);
        meanLabel->setFont(textFont);

        stdLabel = std::make_unique<Label>("stdLabel");
        stdLabel->setBounds(xPos, yPos += textHeight, optionsWidth, textHeight);
        stdLabel->setFont(textFont);

        updateStatLabels();
        rosePlotOptions->addAndMakeVisible(countLabel.get());
        rosePlotOptions->addAndMakeVisible(meanLabel.get());
        rosePlotOptions->addAndMakeVisible(stdLabel.get());

        canvas->addAndMakeVisible(rosePlotOptions.get());

        viewport->setViewedComponent(canvas.get(), false);
        viewport->setScrollBarsShown(true, true);
        viewport->setScrollBarThickness(12);
        addAndMakeVisible(viewport.get());
    }

    Canvas::~Canvas() {}

    void Canvas::paint(Graphics& g)
    {
        g.fillAll(findColour(ThemeColours::componentBackground));
    }

    void Canvas::resized()
    {
        int vpWidth = getWidth();
        int vpHeight = getHeight();
        viewport->setSize(vpWidth, vpHeight);

        int verticalPadding, leftPadding;
        int diameter = getRosePlotDiameter(vpHeight, &verticalPadding);
        int canvasWidth = getContentWidth(vpWidth, diameter, &leftPadding);

        rosePlot->setBounds(leftPadding, verticalPadding, diameter, diameter);

        int optionsX = leftPadding * 2 + diameter;
        int optionsY = verticalPadding + (diameter - minDiameter) / 2;
        rosePlotOptions->setBounds(optionsX, optionsY, optionsWidth, diameter);

        int canvasHeight = diameter + 2 * verticalPadding;
        canvas->setSize(canvasWidth, canvasHeight);
    }

    int Canvas::getRosePlotDiameter(int vpHeight, int* verticalPadding)
    {
        int preferredDiameter = vpHeight - (2 * minPadding);
        int diameter = jmax(minDiameter, jmin(maxDiameter, preferredDiameter));
        if (verticalPadding != nullptr)
        {
            *verticalPadding = jmax(minPadding, (vpHeight - diameter) / 2);
        }
        return diameter;
    }

    int Canvas::getContentWidth(int vpWidth, int diameter, int* leftPadding)
    {
        int widthWithoutPadding = diameter + optionsWidth;
        int lp = jmax(minPadding, jmin(maxLeftPadding, vpWidth - widthWithoutPadding));
        if (leftPadding != nullptr)
        {
            *leftPadding = lp;
        }
        return lp * 2 + widthWithoutPadding;
    }

    void Canvas::refreshState() {}

    void Canvas::updateSettings()
    {
        if(processor->getSelectedStream() != 0)
        {
            // update continuous channel ComboBox to include only active inputs
            Array<int> activeInputs = processor->getActiveChannels();
            int numActiveInputs = activeInputs.size();
            int numItems = cChannelBox->getNumItems();

            DataStream* currStream = processor->getDataStream(processor->getSelectedStream());
            int currContId = (int)currStream->getParameter("vis_cont")->getValue() + 1;
            int currEventId = (int)currStream->getParameter("vis_event")->getValue() + 2;

            cChannelBox->clear(dontSendNotification);

            for (int activeChan = 0; activeChan < numActiveInputs; ++activeChan)
            {
                int chan = activeInputs[activeChan];

                int id = chan + 1;
                cChannelBox->addItem(String(id), id);
                if (id == currContId)
                {
                    cChannelBox->setSelectedId(id, sendNotification);
                }
            }

            if (cChannelBox->getNumItems() > 0 && cChannelBox->getSelectedId() == 0)
            {
                int firstChannelId = activeInputs[0] + 1;
                cChannelBox->setSelectedId(firstChannelId, sendNotification);
            }

            eChannelBox->setSelectedId(currEventId, sendNotification);
        }
    }

    void Canvas::refresh()
    {
        // if no event channel selected, do nothing
        if (eChannelBox->getSelectedId() == 1)
        {
            return;
        }

        // get new angles from visualization phase buffer
        if (processor->tryToReadVisPhases(tempPhaseBuffer))
        {
            // add new angles to rose plot
            while (!tempPhaseBuffer.empty())
            {
                addAngle(tempPhaseBuffer.front());
                tempPhaseBuffer.pop();
            }
        }
    }

    void Canvas::addAngle(double newAngle)
    {
        rosePlot->addAngle(newAngle);
        updateStatLabels();
    }

    void Canvas::clearAngles()
    {
        rosePlot->clear();
        updateStatLabels();
    }

    void Canvas::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
    {
        DataStream* currStream = processor->getDataStream(processor->getSelectedStream());
        if (comboBoxThatHasChanged == cChannelBox.get())
        {
            // subtract 1 to change from 1-based to 0-based
            int newValue = cChannelBox->getSelectedId() - 1;
            currStream->getParameter("vis_cont")->setNextValue(newValue);
        }
        else if (comboBoxThatHasChanged == eChannelBox.get())
        {
            // subtract 2, since index 1 == no channel (-1)
            int newValue = eChannelBox->getSelectedId() - 2;
            currStream->getParameter("vis_event")->setNextValue(newValue);
        }
    }

    void Canvas::sliderValueChanged(Slider* slider)
    {
        if (slider == numBinsSlider.get())
        {
            rosePlot->setNumBins(static_cast<int>(slider->getValue()));
        }
    }

    void Canvas::buttonClicked(Button* button)
    {
        if (button == clearButton.get())
        {
            clearAngles();
        }
    }

    void Canvas::updateStatLabels()
    {
        int numAngles = rosePlot->getNumAngles();
        double mean = std::round(100 * rosePlot->getCircMean()) / 100;
        double stddev = std::round(100 * rosePlot->getCircStd()) / 100;

        countLabel->setText("Events received: " + String(numAngles), dontSendNotification);
        meanLabel->setText("Mean phase (vs. reference): " + String(mean) + "\u00b0", dontSendNotification);
        stdLabel->setText("Standard deviation phase: " + String(stddev) + "\u00b0", dontSendNotification);
    }

    void Canvas::saveCustomParametersToXml(XmlElement* xml)
    {
        XmlElement* visValues = xml->createNewChildElement("VISUALIZER");
        visValues->setAttribute("eventChannelId", eChannelBox->getSelectedId());
        visValues->setAttribute("numBins", numBinsSlider->getValue());
        visValues->setAttribute("phaseRef", referenceEditable->getText());
    }

    void Canvas::loadCustomParametersFromXml(XmlElement* xml)
    {
        for(auto xmlNode : xml->getChildWithTagNameIterator("VISUALIZER"))
        {
            int eventChannelId = xmlNode->getIntAttribute("eventChannelId", eChannelBox->getSelectedId());
            if (eChannelBox->indexOfItemId(eventChannelId) != -1)
            {
                eChannelBox->setSelectedId(eventChannelId, sendNotificationSync);
            }

            numBinsSlider->setValue(xmlNode->getDoubleAttribute("numBins", numBinsSlider->getValue()), sendNotificationSync);
            referenceEditable->setText(xmlNode->getStringAttribute("phaseRef", referenceEditable->getText()), sendNotificationSync);
        }
    }

    void Canvas::displayContinuousChan(int chan)
    {
        if (cChannelBox->indexOfItemId(chan + 1) == -1)
        {
            jassertfalse;
            return;
        }
        // remember to switch to 1-based
        cChannelBox->setSelectedId(chan + 1, dontSendNotification);
    }

    /**** RosePlot ****/

    RosePlot::RosePlot(Canvas* c)
        : canvas(c)
        , referenceAngle(static_cast<double>(startReference))
        , numBins(startNumBins)
        , edgeWeight(1)
        , rSum(0)
    {
        updateAngles();
        reorganizeAngleData();
    }

    RosePlot::~RosePlot() {}

    void RosePlot::paint(Graphics& g)
    {
        // dimensions
        juce::Rectangle<float> bounds = getBounds().toFloat();
        float squareSide = jmin(bounds.getHeight(), bounds.getWidth() - 2 * textBoxSize);
        juce::Rectangle<float> plotBounds = bounds.withZeroOrigin();
        plotBounds = plotBounds.withSizeKeepingCentre(squareSide, squareSide);
        g.setColour(findColour(ThemeColours::widgetBackground));
        g.fillEllipse(plotBounds);

        // draw grid
        // spokes and degree labels (every 30 degrees)
        juce::Point<float> center = plotBounds.getCentre();
        Line<float> spoke(center, center);
        juce::Rectangle<int> textBox(textBoxSize, textBoxSize);
        g.setFont(Font(textBoxSize / 2, Font::bold));
        for (int i = 0; i < 12; ++i)
        {
            float juceAngle = i * float_Pi / 6;
            spoke.setEnd(center.getPointOnCircumference(squareSide / 2, juceAngle));
            g.setColour(findColour(ThemeColours::defaultFill));
            g.drawLine(spoke);

            float textRadius = (squareSide + textBoxSize) / 2;
            juce::Point<int> textCenter = center.getPointOnCircumference(textRadius, juceAngle).toInt();
            int degreeAngle = (450 - 30 * i) % 360;
            g.setColour(findColour(ThemeColours::defaultText));
            g.drawFittedText(String(degreeAngle), textBox.withCentre(textCenter), Justification::centred, 1);
        }

        // concentric circles
        int nCircles = 3;
        juce::Rectangle<float> circleBounds;
        g.setColour(findColour(ThemeColours::defaultFill));
        for (int i = 1; i < nCircles; ++i)
        {
            float diameter = (squareSide * i) / nCircles;
            circleBounds = plotBounds.withSizeKeepingCentre(diameter, diameter);
            g.drawEllipse(circleBounds, 1);
        }

        // get count for each rose plot segment
        int nSegs = binMidpoints.size();
        Array<int> segmentCounts;
        segmentCounts.resize(nSegs);
        int maxCount = 0;
        int totalCount = 0;
        for (int seg = 0; seg < nSegs; ++seg)
        {
            int count = static_cast<int>(angleData->count(binMidpoints[seg] + referenceAngle));
            segmentCounts.set(seg, count);
            maxCount = jmax(maxCount, count);
            totalCount += count;
        }

        jassert(totalCount == angleData->size());
        jassert((maxCount == 0) == (angleData->empty()));

        // construct path
        Path rosePath;
        for (int seg = 0; seg < nSegs; ++seg)
        {
            if (segmentCounts[seg] == 0)
            {
                continue;
            }

            float size = squareSide * segmentCounts[seg] / static_cast<float>(maxCount);
            rosePath.addPieSegment(plotBounds.withSizeKeepingCentre(size, size),
                segmentAngles[seg].first, segmentAngles[seg].second, 0);
        }

        // paint path
        g.setColour(findColour(ThemeColours::highlightedFill));
        g.fillPath(rosePath);
        g.setColour(findColour(ThemeColours::widgetBackground));
        g.strokePath(rosePath, PathStrokeType(edgeWeight));
    }

    void RosePlot::setNumBins(int newNumBins)
    {
        if (newNumBins != numBins && newNumBins > 0 && newNumBins <= maxBins)
        {
            numBins = newNumBins;
            updateAngles();
            reorganizeAngleData();
            repaint();
        }
    }

    void RosePlot::setReference(double newReference)
    {
        if (newReference != referenceAngle)
        {
            referenceAngle = newReference;
            reorganizeAngleData();
            repaint();
        }
    }

    void RosePlot::addAngle(double newAngle)
    {
        newAngle = Node::circDist(newAngle, 0.0);
        angleData->insert(newAngle);
        rSum += std::exp(std::complex<double>(0, newAngle));
        repaint();
    }

    void RosePlot::clear()
    {
        angleData->clear();
        rSum = 0;
        repaint();
    }

    int RosePlot::getNumAngles()
    {
        return static_cast<int>(angleData->size());
    }

    double RosePlot::getCircMean(bool usingReference)
    {
        if (angleData->empty())
        {
            return 0;
        }

        double reference = usingReference ? referenceAngle : 0.0;
        // use range of (-90, 270] for ease of use
        double meanRad = Node::circDist(std::arg(rSum), reference, 3 * double_Pi / 2);
        return radiansToDegrees(meanRad);
    }

    double RosePlot::getCircStd()
    {
        if (angleData->empty())
        {
            return 0;
        }

        double r = std::abs(rSum) / angleData->size();
        double stdRad = std::sqrt(-2 * std::log(r));
        return radiansToDegrees(stdRad);
    }

    void RosePlot::labelTextChanged(Label* labelThatHasChanged)
    {
        if (labelThatHasChanged->getName() == "referenceEditable")
        {
            double doubleInput;
            double currReferenceDeg = radiansToDegrees(referenceAngle);
            // bool valid = Editor::updateControl(labelThatHasChanged,
                // -DBL_MAX, DBL_MAX, currReferenceDeg, doubleInput);

            if (true)
            {
                // convert to radians
                double newReference = Node::circDist(degreesToRadians(doubleInput), 0.0);
                labelThatHasChanged->setText(String(radiansToDegrees(newReference)), dontSendNotification);
                setReference(newReference);
                canvas->updateStatLabels();
            }
        }
    }


    /*** RosePlot private members ***/

    RosePlot::AngleDataMultiset::AngleDataMultiset(int numBins, double referenceAngle)
        : std::multiset<double, std::function<bool(double, double)>>(
        std::bind(circularBinCompare, numBins, referenceAngle, std::placeholders::_1, std::placeholders::_2))
    {}

    RosePlot::AngleDataMultiset::AngleDataMultiset(int numBins, double referenceAngle, AngleDataMultiset* dataSource)
        : AngleDataMultiset(numBins, referenceAngle)
    {
        insert(dataSource->begin(), dataSource->end());
    }


    bool RosePlot::AngleDataMultiset::circularBinCompare(int numBins, double referenceAngle, double lhs, double rhs)
    {
        double lhsDist = Node::circDist(lhs, referenceAngle);
        double rhsDist = Node::circDist(rhs, referenceAngle);
        int lhsBin = int(lhsDist * numBins / (2 * double_Pi));
        int rhsBin = int(rhsDist * numBins / (2 * double_Pi));
        return lhsBin < rhsBin;
    }

    void RosePlot::reorganizeAngleData()
    {
        AngleDataMultiset* newAngleData;
        if (angleData == nullptr)
        {
            // construct empty container
            newAngleData = new AngleDataMultiset(numBins, referenceAngle);
        }
        else
        {
            // copy existing data to new container
            newAngleData = new AngleDataMultiset(numBins, referenceAngle, angleData.get());
        }

        angleData.reset(newAngleData);
    }

    void RosePlot::updateAngles()
    {
        float step = 2 * float_Pi / numBins;
        binMidpoints.resize(numBins);
        for (int i = 0; i < numBins; ++i)
        {
            binMidpoints.set(i, step * (i + 0.5));
            float firstAngle = float(Node::circDist(double_Pi / 2, step * (i + 1)));
            segmentAngles.set(i, { firstAngle, firstAngle + step });
        }
    }
}