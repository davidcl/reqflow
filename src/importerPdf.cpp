
#include <fstream>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-global.h>
typedef std::vector<char> byte_array;


#include "importerPdf.h"
#include "logging.h"
#include "req.h"

void ReqDocumentPdf::dumpText(const char *file, Encoding encoding)
{
	poppler::document *doc = poppler::document::load_from_file(file);
	if (!doc) {
		LOG_ERROR("Cannot open file: %s", file);
		return;
	}
	const int pagesNbr = doc->pages();
	LOG_DEBUG("loadPdf: page count: %d", pagesNbr);
	
	for (int i = 0; i < pagesNbr; ++i) {
        switch(encoding) {
        case LATIN1:
            printf("%s", doc->create_page(i)->text().to_latin1().c_str());
            break;
        case UTF8:
        default:
        {
            byte_array pageText = doc->create_page(i)->text().to_utf8();
            std::string pageLines(pageText.begin(), pageText.end());
			printf("%s", pageLines.c_str());
            break;
        }
        }
	}
}

int ReqDocumentPdf::loadRequirements()
{
    LOG_DEBUG("ReqDocumentPdf::loadRequirements: %s", fileConfig.path.c_str());

	poppler::document *doc = poppler::document::load_from_file(fileConfig.path.c_str());
	if (!doc) {
		LOG_ERROR("Cannot open file: %s", fileConfig.path.c_str());
        return -1;
	}
	const int pagesNbr = doc->pages();
	LOG_DEBUG("loadPdf: page count: %d", pagesNbr);

    started = true;
	if (fileConfig.startAfterRegex) started = false;
    currentRequirement = "";

	for (int i = 0; i < pagesNbr; ++i) {
		LOG_DEBUG("page %d", i+1);
		// process the lines
		std::string pageLines;

		switch(fileConfig.encoding) {
		case LATIN1:
			pageLines = doc->create_page(i)->text().to_latin1();
			break;
		case UTF8:
		default:
		{
			byte_array pageText = doc->create_page(i)->text().to_utf8();
			pageLines.assign(pageText.begin(), pageText.end());
			break;
		}
		}

        // process one line at a time
		const char *startOfLine = pageLines.c_str();
		const char *endOfLine = 0;
		while (startOfLine) {
			endOfLine = strstr(startOfLine, "\n");
			int length = 0;
			if (endOfLine) length = endOfLine - startOfLine;
			else length = strlen(startOfLine);

			std::string line;
			line.assign(startOfLine, length);

            BlockStatus status = processBlock(line);
            if (status == STOP_REACHED) {
                delete doc;
                return 0;
            }

			if (!endOfLine) break;
			if (endOfLine == startOfLine + strlen(startOfLine) - 1) break;
			startOfLine = endOfLine + 1;
		}
	}
	delete doc;
    return 0;
}
