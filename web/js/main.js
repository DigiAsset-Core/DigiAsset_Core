(function () {
    "use strict";
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
    }

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
            selectDOM.forEach(e => e.addEventListener(type, listener));
        } else {
            if (selectDOM) {
                selectDOM.addEventListener(type, listener);
            } else {
                console.warn(`No element found with selector "${DOM}" to attach event "${type}".`);
            }
        }
    }
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
        const positionCommand = position === "start" ? 'afterbegin' : 'beforeend';

        // Check if elements is iterable (like a NodeList)
        if (NodeList.prototype.isPrototypeOf(elements)) {
            elements.forEach(element => {
                if (element.insertAdjacentHTML) {
                    element.insertAdjacentHTML(positionCommand, htmlContent);
                }
            });
        } else if (elements && elements.insertAdjacentHTML) {
            // Single Element
            elements.insertAdjacentHTML(positionCommand, htmlContent);
        } else {
            console.error('The provided target is neither an Element nor a NodeList.');
        }
    }

    // Add copy button for code blocks
    document.on('DOMContentLoaded', () => {
        let codeBlocks = select("pre.literal-block", true);

        codeBlocks.forEach(block => {
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
                let newLeftPosition = Math.min(block.scrollLeft + blockRect.width - btnWidth - rightSpace, maxLeftPosition);

                btn.style.left = `${newLeftPosition}px`;
            };

            updateHeaderWidth();
            updateButtonPosition(); // Update position initially

            window.addEventListener('resize', () => {
                updateHeaderWidth();
                updateButtonPosition(); // Re-calculate position on resize
            });

            block.addEventListener('scroll', updateButtonPosition);

            btn.onclick = function () {
                navigator.clipboard.writeText(block.textContent).then(() => {
                    console.log("Text copied to clipboard");
                    btn.textContent = "Copied!";
                    setTimeout(() => { btn.textContent = 'Copy'; }, 2000);
                }).catch(err => {
                    console.error("Failed to copy text: ", err);
                });
            };
        });
    })
})();
