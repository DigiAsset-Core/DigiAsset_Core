// MIT License

// Copyright (c) 2024 Oliver Krilov

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/**
 * Selects DOM elements based on a CSS selector.
 *
 * @param {string} DOM - The CSS selector string used to identify the element(s).
 * @param {boolean} [all=false] - If true, selects all elements that match the selector.
 *                                Otherwise, selects only the first element that matches the selector.
 * @returns {Element|NodeList} - The selected element if 'all' is false, or a NodeList of elements if 'all' is true.
 */
const select = (DOM, all = false) => {
    DOM = DOM.trim();
    if (all) return [...document.querySelectorAll(DOM)];
    else return document.querySelector(DOM);
};

/**
 * Adds an event listener to elements selected by a CSS selector.
 *
 * @param {string} type - The type of event to listen for (e.g., 'click', 'mouseover').
 * @param {string} DOM - The CSS selector string used to identify the element(s) to attach the listener to.
 * @param {Function} listener - The event handler function that will be called when the event is triggered.
 * @param {boolean} [all=false] - If true, adds the event listener to all elements matching the selector.
 *                                Otherwise, adds it only to the first matching element.
 */
const on = (type, DOM, listener, all = false) => {
    let selectDOM = select(DOM, all);
    if (all) {
        selectDOM.forEach((e) => e.addEventListener(type, listener));
    } else {
        if (selectDOM) {
            selectDOM.addEventListener(type, listener);
        } else {
            console.warn(
                `No element found with selector "${DOM}" to attach event "${type}".`
            );
        }
    }
};
/**
 * Extends the Document prototype to include an 'on' method for adding event listeners.
 */
Document.prototype.on = function (type, listener, options) {
    this.addEventListener(type, listener, options);
};

/**
 * Appends HTML content to a DOM element or each element in a NodeList at a specified position.
 *
 * @param {Element|NodeList} elements - The element or NodeList to which the HTML content will be appended.
 * @param {string} htmlContent - The HTML content to append.
 * @param {string} [position="end"] - The position to insert the HTML ("start" or "end").
 */
function appendHTML(elements, htmlContent, position = "end") {
    const positionCommand = position === "start" ? "afterbegin" : "beforeend";

    // Check if elements is iterable (like a NodeList)
    if (NodeList.prototype.isPrototypeOf(elements)) {
        elements.forEach((element) => {
            if (element.insertAdjacentHTML) {
                element.insertAdjacentHTML(positionCommand, htmlContent);
            }
        });
    } else if (elements && elements.insertAdjacentHTML) {
        // Single Element
        elements.insertAdjacentHTML(positionCommand, htmlContent);
    } else {
        console.error(
            "The provided target is neither an Element nor a NodeList."
        );
    }
}

/**
 * Get or set CSS styles on a DOM element.
 *
 * @param {Element} element - The DOM element to get or set styles.
 * @param {string|object} styleNameOrObject - A CSS property name as a string to get its value,
 *                                            or an object with style properties to set on the element.
 * @param {string} [value] - The value to set for the property (if styleNameOrObject is a string).
 */
function css(element, styleNameOrObject, value) {
    if (typeof styleNameOrObject === "string") {
        if (typeof value === "undefined") {
            // Get the style value
            return getComputedStyle(element)[styleNameOrObject];
        } else {
            // Set the style value
            element.style[styleNameOrObject] = value;
        }
    } else if (typeof styleNameOrObject === "object") {
        // Set multiple styles
        for (let property in styleNameOrObject) {
            element.style[property] = styleNameOrObject[property];
        }
    }
}

/**
 * Checks if the given object is a DOM element or HTML document.
 *
 * This function determines if the passed object is an instance of `Element` 
 * or `HTMLDocument`, which are both commonly used DOM interfaces. It returns
 * true if the object is one of these types, making it useful for ensuring that
 * functions working with the DOM are provided with proper element types.
 *
 * @param {Object} obj - The object to test against the Element and HTMLDocument classes.
 * @returns {boolean} True if the object is an instance of Element or HTMLDocument, false otherwise.
 */
const isElement = (obj) => {
    return obj instanceof Element || obj instanceof HTMLDocument;
}

/**
 * Checks if the specified element has the given class name.
 *
 * This function determines if the specified DOM element contains a specific class
 * within its class attribute. It uses a regular expression to search for the class name
 * within the element's className string, ensuring it matches exactly and not as a substring
 * of another class name.
 *
 * @param {Element} el - The DOM element to check for the class.
 * @param {string} cls - The class name to search for within the element's class attribute.
 * @returns {boolean} True if the element has the specified class, false otherwise.
 */
const hasClass = (el, cls) => {
    return !!el.className.match(new RegExp('(\\s|^)' + cls + '(\\s|$)'));
}

(function () {
    "use strict";

    // Add copy button for code blocks
    document.on("DOMContentLoaded", () => {
        let codeBlocks = select("pre.literal-block", true);

        codeBlocks.forEach((block) => {
            let header = document.createElement("div");
            header.className = "code-header";
            let btn = document.createElement("button");
            btn.className = "copy-button";
            btn.textContent = "Copy";
            header.appendChild(btn);
            block.prepend(header);

            const updateHeaderWidth = () => {
                header.style.width = `${block.scrollWidth}px`;
            };

            const updateButtonPosition = () => {
                let blockRect = block.getBoundingClientRect();
                let btnWidth = btn.offsetWidth;
                let rightSpace = 10; // Space from the right end of the header

                // Calculate new left position, ensuring button doesn't overshoot the header width
                let maxLeftPosition = block.scrollWidth - btnWidth - rightSpace;
                let newLeftPosition = Math.min(
                    block.scrollLeft + blockRect.width - btnWidth - rightSpace,
                    maxLeftPosition
                );

                btn.style.left = `${newLeftPosition}px`;
            };

            updateHeaderWidth();
            updateButtonPosition(); // Update position initially

            window.addEventListener("resize", () => {
                updateHeaderWidth();
                updateButtonPosition(); // Re-calculate position on resize
            });

            block.addEventListener("scroll", updateButtonPosition);

            btn.onclick = function () {
                navigator.clipboard
                    .writeText(
                        block.textContent.substr(4, block.textContent.length)
                    )
                    .then(() => {
                        console.log("Text copied to clipboard");
                        btn.textContent = "Copied!";
                        setTimeout(() => {
                            btn.textContent = "Copy";
                        }, 2000);
                    })
                    .catch((err) => {
                        console.error("Failed to copy text: ", err);
                    });
            };
        });
    });

    document.addEventListener("DOMContentLoaded", () => {
        let copyrightElement = select("footer .copyright");
        if (!copyrightElement) return console.log("No footer copyright found!");
    
        let currYear = new Date().getFullYear(); // Correct method invocation
        let copyrightText = copyrightElement.textContent; // Get the text content of the element
    
        // Assuming the year is the last 4 characters of the text
        let copyrightYear = copyrightText.slice(-4); // Use slice to get the last four characters
        console.log(copyrightYear);
    
        // Parse as integers to compare
        let currentYearInt = parseInt(currYear, 10);
        let copyrightYearInt = parseInt(copyrightYear, 10);
    
        if (!isNaN(currentYearInt) && !isNaN(copyrightYearInt) && currentYearInt > copyrightYearInt) {
            copyrightElement.innerHTML += ` - ${currentYearInt}`; // Append current year
        }
    });
    
})();
