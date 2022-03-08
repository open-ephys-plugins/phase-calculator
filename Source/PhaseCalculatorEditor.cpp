/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
Copyright (C) 2017 Translational NeuroEngineering Laboratory, MGH

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

#include "PhaseCalculatorEditor.h"
#include "PhaseCalculatorCanvas.h"
#include "HTransformers.h"
#include <climits> // INT_MAX
#include <cfloat>  // FLT_MAX
#include <cmath>   // abs

namespace PhaseCalculator
{
    Editor::Editor(Node* parentNode)
        : VisualizerEditor(parentNode, 300)
    {
        tabText = "Event Phase Plot";
        int filterWidth = 120;

        // make the canvas now, so that restoring its parameters always works.
        canvas = std::make_unique<Canvas>(parentNode);

        addComboBoxParameterEditor("freq_band", 5, 25);

        addTextBoxParameterEditor("low_cut", 5, 70);

        addTextBoxParameterEditor("high_cut", 5, 100);

        addTextBoxParameterEditor("recalc_ar", filterWidth, 25);

        addTextBoxParameterEditor("ar_order", filterWidth, 65);

        addComboBoxParameterEditor("output_mode", filterWidth, 87);

        addSelectedChannelsParameterEditor("Channels", filterWidth + 80, 30);

    }

    Editor::~Editor() {}

    // void Editor::channelChanged(int chan, bool newState)
    // {
    //     auto pc = static_cast<Node*>(getProcessor());
    //     if (chan < pc->getNumInputs())
    //     {
    //         // mark channel as activated or deactivated
    //         if (newState)
    //         {
    //             // check whether channel can be activated
    //             if (!pc->activateInputChannel(chan))
    //             {
    //                 pc->deselectChannel(chan, true);
    //                 return;
    //             }
    //         }
    //         else
    //         {
    //             pc->deactivateInputChannel(chan);
    //         }
            
    //         // update visualizer and maybe extra output channels
    //         if (pc->outputMode == PH_AND_MAG)
    //         {
    //             if (newState)
    //             {
    //                 extraChanManager.addExtraChan(chan);
    //             }
    //             else
    //             {
    //                 extraChanManager.removeExtraChan(chan);
    //             }

    //             // Update signal chain to add/remove output channels if necessary
    //             CoreServices::updateSignalChain(this);
    //         }
    //         else // (if not updating the whole signal chain)
    //         {
    //             // update the available continuous channels for visualizer
    //             updateVisualizer();
    //         }
    //     }
    // }

    Visualizer* Editor::createNewCanvas()
    {
        return canvas.get();
    }

    void Editor::saveVisualizerEditorParameters(XmlElement* xml)
    {
        // xml->setAttribute("Type", "PhaseCalculatorEditor");
        // Node* processor = (Node*)(getProcessor());

        // XmlElement* paramValues = xml->createNewChildElement("VALUES");
        // paramValues->setAttribute("calcInterval", processor->calcInterval);
        // paramValues->setAttribute("arOrder", processor->arOrder);
        // paramValues->setAttribute("lowCut", processor->lowCut);
        // paramValues->setAttribute("highCut", processor->highCut);
        // paramValues->setAttribute("outputMode", processor->outputMode);

        // const Array<float>& validBand = Hilbert::validBand[processor->band];
        // paramValues->setAttribute("rangeMin", validBand[0]);
        // paramValues->setAttribute("rangeMax", validBand[1]);
    }

    void Editor::loadVisualizerEditorParameters(XmlElement* xml)
    {
        // forEachXmlChildElementWithTagName(*xml, xmlNode, "VALUES")
        // {
        //     // some parameters have two fallbacks for backwards compatability
        //     recalcIntervalEditable->setText(xmlNode->getStringAttribute("calcInterval", recalcIntervalEditable->getText()), sendNotificationSync);
        //     arOrderEditable->setText(xmlNode->getStringAttribute("arOrder", arOrderEditable->getText()), sendNotificationSync);
        //     bandBox->setSelectedId(selectBandFromSavedParams(xmlNode) + 1, sendNotificationSync);
        //     lowCutEditable->setText(xmlNode->getStringAttribute("lowCut", lowCutEditable->getText()), sendNotificationSync);
        //     highCutEditable->setText(xmlNode->getStringAttribute("highCut", highCutEditable->getText()), sendNotificationSync);
        //     outputModeBox->setSelectedId(xmlNode->getIntAttribute("outputMode", outputModeBox->getSelectedId()), sendNotificationSync);
        // }
    }

    // static utilities

    int Editor::selectBandFromSavedParams(const XmlElement* xmlNode)
    {
        if (xmlNode->hasAttribute("rangeMin") && xmlNode->hasAttribute("rangeMax"))
        {
            // I don't trust JUCE's string parsing functionality (doesn't indicate failure, etc...)
            float rangeMin, rangeMax;
            if (readNumber(xmlNode->getStringAttribute("rangeMin"), rangeMin) &&
                readNumber(xmlNode->getStringAttribute("rangeMax"), rangeMax))
            {
                // try to find a matching band.
                for (int band = 0; band < NUM_BANDS; ++band)
                {
                    const Array<float>& validRange = Hilbert::validBand[band];

                    if (validRange[0] == rangeMin && validRange[1] == rangeMax)
                    {
                        return band;
                    }
                }
            }
        }

        // no match, or rangeMin and rangeMax were invalid
        // try to find a good fit for lowCut and highCut
        if (xmlNode->hasAttribute("lowCut") && xmlNode->hasAttribute("highCut"))
        {
            float lowCut, highCut;
            if (readNumber(xmlNode->getStringAttribute("lowCut"), lowCut) && lowCut >= 0 &&
                readNumber(xmlNode->getStringAttribute("highCut"), highCut) && highCut > lowCut)
            {
                float midCut = (lowCut + highCut) / 2;
                int bestBand = 0;
                float bestBandDist = FLT_MAX;

                for (int band = 0; band < NUM_BANDS; ++band)
                {
                    const Array<float>& validRange = Hilbert::validBand[band];

                    float midBand = (validRange[0] + validRange[1]) / 2;
                    float midDist = std::abs(midBand - midCut);
                    if (validRange[0] <= lowCut && validRange[1] >= highCut && midDist < bestBandDist)
                    {
                        bestBand = band;
                        bestBandDist = midDist;
                    }
                }
                return bestBand;
            }
        }

        // just fallback to first band
        return 0;
    }
}