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
// Extending Element.prototype to support 'on' and 'css' methods directly on HTML elements.
Element.prototype.on = function (type, listener, options) {
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

Element.prototype.css = function (styles) {
    for (const property in styles) {
        this.style[property] = styles[property];
    }
};

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

const getCcpFilePath = () => {
    let currentUrl = window.location.href; // Get current url
    let urlObj = new URL(currentUrl); // Create URL object for handling

    let pathname = urlObj.pathname.startsWith('/') ? urlObj.pathname.substring(1) : urlObj.pathname; // Remove leading slash

    if (pathname.startsWith('src/')) {
        let cppPathname = pathname.replace('.html', '.cpp');

        return `${urlObj.origin}/${cppPathname}`; // Return corresponding cpp path
    } else return false; //Returns boolean value false if not in /src
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
                let rightSpace = 20; // Space from the right end of the header

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

    // Update footer copyright year
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

    // Add source code to documentation page
    if (getCcpFilePath()) {
        let ccpFilePath = getCcpFilePath();
        console.log(`CPP File path found: ${ccpFilePath}`)

        let card = document.createElement("div");
        let cardHeader = document.createElement("div");
        let cardTitle = document.createElement("h2");
        let rawBtn = document.createElement("button");
        let rawLink = document.createElement("a");
        let cardBody = document.createElement("div");
        let code = document.createElement("pre");
        let cardFooter = document.createElement("div")

        cardHeader.on('click', () => {
            let cls = "collapsed";
            if (!hasClass(cardBody, cls)) cardBody.classList.add(cls)
            else cardBody.classList.remove(cls);
        })

        cardHeader.classList.add("ch");
        card.classList.add("card");
        cardBody.classList.add("cb");
        cardBody.classList.add("collapsed");
        code.classList.add("literal-block");
        cardFooter.classList.add("cf");

        cardTitle.textContent = "Source Code";
        rawLink.textContent = "Raw";

        rawBtn.append(rawLink);
        cardHeader.append(cardTitle);
        cardFooter.append(rawBtn);
        card.append(cardHeader);

        rawLink.href = ccpFilePath;
        rawLink.setAttribute('target', '_blank');
        css(rawBtn, 'float', 'right');
        rawLink.css('display', 'block');

        fetch(ccpFilePath)
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok ' + response.statusText);
                }
                return response.text();
            })
            .then(data => { 
                code.textContent = data;
            })
            .catch(error => {
                console.error('Error fetching file:', error);
                code.textContent = 'Failed to load content.';
            });

        cardBody.append(code);
        card.append(cardBody);
        card.append(cardFooter);

        select(".document", false).append(card);
    }
    
    document.addEventListener("DOMContentLoaded", function () {
        const links = document.querySelectorAll(".sidebar a");
        const mainContent = document.querySelector("#main-content");
        const loader = document.getElementById("loader");
        const body = document.body;

        // Modify link behavior to scroll instead of navigating
        links.forEach((link) => {
            link.addEventListener("click", function (e) {
                if (e.ctrlKey || e.metaKey || e.button === 1) {
                    // Allow ctrl/cmd+click or middle mouse to open link in new tab
                    return; // Do nothing, let the browser handle it
                }
                e.preventDefault(); // Prevent default link behavior
                const sectionId = this.getAttribute("href")
                    .split("/")
                    .pop()
                    .replace(".html", ""); // Extract section ID from href
                const section = document.getElementById(sectionId);
                if (section) {
                    section.scrollIntoView({
                        behavior: "smooth",
                        block: "start",
                    }); // Smoothly scroll to the section
                }
            });
        });

        // Function to hide loader and allow scrolling
        function hideLoader() {
            loader.style.display = "none";
            body.classList.remove("noscroll");
            mainContent.style.overflow = "auto"; // Ensure main content can scroll
        }

        // Function to show loader and prevent scrolling
        function showLoader() {
            loader.style.display = "block";
            body.classList.add("noscroll");
            mainContent.style.overflow = "hidden"; // Prevent scrolling in main content
        }

        // Initially show the loader
        showLoader();

        // Fetch and append content from each link
        let contentPromises = Array.from(links).map((link) => {
            return fetch(link.href)
                .then((response) => response.text())
                .then((html) => {
                    const parser = new DOMParser();
                    const doc = parser.parseFromString(
                        html,
                        "text/html"
                    );
                    const docContent = doc.querySelector(".document");

                    if (docContent) {
                        const section =
                            document.createElement("section");
                        section.id = link.href
                            .split("/")
                            .pop()
                            .split(".")[0]; // Assuming URL structure to extract ID
                        section.innerHTML = docContent.innerHTML;
                        mainContent.appendChild(section);
                        observer.observe(section); // Observe the new section
                    }
                })
                .catch((error) =>
                    console.error("Error loading the content:", error)
                );
        });

        // Hide loader when all content is loaded
        Promise.all(contentPromises).then(() => {
            hideLoader();
        });

        // Intersection Observer to update active link on scroll
        const observer = new IntersectionObserver(
            (entries) => {
                entries.forEach((entry) => {
                    if (entry.isIntersecting) {
                        links.forEach((a) => {
                            const parentLi = a.parentElement;
                            const hrefParts = a.href.split("/");
                            const lastPart = hrefParts[
                                hrefParts.length - 1
                            ].replace(".html", ""); // Assuming the URLs end with '.html'

                            if (lastPart === entry.target.id) {
                                parentLi.classList.add("active");
                            } else {
                                parentLi.classList.remove("active");
                            }
                        });
                    }
                });
            },
            { threshold: 0.5 }
        );

        // Sidebar toggle management
        const sidebar = document.getElementById("sidebar");
        const toggleButton = document.createElement("button");
        toggleButton.textContent = "☰";
        toggleButton.classList.add("toggle-sidebar");
        document.body.appendChild(toggleButton);

        // Function to open the sidebar
        function openSidebar() {
            const sidebar = document.getElementById("sidebar");
            sidebar.classList.add("open");
        }

        // Function to close the sidebar
        function closeSidebar() {
            const sidebar = document.getElementById("sidebar");
            sidebar.classList.remove("open");
        }

        function toggleSidebar() {
            sidebar.classList.toggle("open");
            body.classList.toggle(
                "noscroll",
                sidebar.classList.contains("open")
            );
        }

        toggleButton.addEventListener("click", toggleSidebar);

        mainContent.addEventListener("click", function () {
            if (sidebar.classList.contains("open")) {
                toggleSidebar(); // Close sidebar if open when clicking main content
            }
        });

        // Adjust UI on window resize
        window.addEventListener("resize", function () {
            adjustUIForWindowSize();
        });

        // Function to adjust UI based on window size
        function adjustUIForWindowSize() {
            if (window.innerWidth >= 576) {
                openSidebar(); // Automatically open the sidebar on larger screens
            } else if (!sidebar.contains(document.activeElement)) {
                closeSidebar(); // Automatically close the sidebar on smaller screens
            }
        }

        // Initial adjustment on load
        adjustUIForWindowSize();
    });
})();
